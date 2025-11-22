#!/usr/bin/env python3
import csv
import sys
import re
from statistics import mean

def safe_float(s):
    s = s.strip()
    if s == "":
        return None
    s_clean = re.sub(r"[^\d\.\-eE+]", "", s)
    try:
        return float(s_clean)
    except:
        return None

def find_columns(header):
    # header: list of strings
    time_col = None
    level_col = None
    for i, h in enumerate(header):
        lh = h.lower()
        if "micro" in lh or "time" in lh:
            time_col = i
        if "logic" in lh or "channel" in lh or lh in ("0","1","d0","d1"):
            # prefer explicit logic label
            level_col = i
    return time_col, level_col

def read_samples(filename):
    times = []
    levels = []
    with open(filename, newline='') as f:
        reader = csv.reader(f)
        header = next(reader, None)
        if header is None:
            raise SystemExit("CSV vacío.")
        time_col, level_col = find_columns(header)
        # if header looks like a single-column file without header, try to detect later
        # if we couldn't find columns in header, assume standard ordering: time, logic, ...
        if time_col is None or level_col is None:
            # try to infer from first data row by peeking
            first_row = None
            # attempt to parse rest until find a numeric row
            for row in reader:
                if not row:
                    continue
                first_row = row
                break
            if first_row is None:
                raise SystemExit("No hay filas de datos en el CSV.")
            # heurística: if first row has >1 columns and first is numeric, assume time at 0 and logic at 1
            if len(first_row) >= 2 and safe_float(first_row[0]) is not None and first_row[1] in ("0","1"):
                time_col = 0
                level_col = 1
                # process that first row and continue with rest
                times.append(float(first_row[time_col]))
                levels.append(int(first_row[level_col]))
            else:
                # if header was actually data (e.g., file started with "logic" then values) -> treat file as single-column logic
                # reopen file and treat as single column of logic with implicit equal spacing (not ideal)
                f.seek(0)
                reader = csv.reader(f)
                for row in reader:
                    if not row:
                        continue
                    val = row[0].strip()
                    if val in ("0","1"):
                        levels.append(int(val))
                # if we only have levels, we cannot compute times; raise
                raise SystemExit("CSV solo contiene niveles sin timestamps. Necesitamos la columna de tiempo (microseconds o Time).")
        else:
            # normal path: parse remainder rows including header maybe
            for row in reader:
                if len(row) <= max(time_col, level_col):
                    continue
                t = safe_float(row[time_col])
                if t is None:
                    continue
                lvl = row[level_col].strip()
                if lvl not in ("0","1"):
                    # try numeric parse
                    if safe_float(lvl) is not None:
                        lvl = "1" if float(lvl) != 0 else "0"
                    else:
                        continue
                times.append(float(t))
                levels.append(int(lvl))
    return times, levels

def group_segments(times, levels, time_is_micro=True):
    if not times or not levels or len(times) != len(levels):
        raise ValueError("Datos inválidos")
    # If time column is in seconds (small floats with dots), detect: heuristic: if max time < 1000 -> seconds probably
    # But user has 'microseconds' header, so assume microseconds if values > 1e3.
    # For safety, convert to microseconds if values look like seconds:
    times_us = times[:]
    # if majority of times < 1e3 -> likely seconds -> convert
    if max(times_us) < 1e3:
        times_us = [t * 1_000_000.0 for t in times_us]
    # Now group consecutive identical levels
    seg_durations = []  # durations in microseconds
    seg_levels = []
    start_time = times_us[0]
    cur_lvl = levels[0]
    for i in range(1, len(times_us)):
        if levels[i] != cur_lvl:
            # segment from start_time to times_us[i] (start of next)
            dur = int(round(times_us[i] - start_time))
            seg_durations.append(dur)
            seg_levels.append(cur_lvl)
            # start new segment
            start_time = times_us[i]
            cur_lvl = levels[i]
    # final segment: from start_time to last sample time + estimated sample period
    # estimate sample period as median delta
    deltas = [times_us[i+1]-times_us[i] for i in range(len(times_us)-1)]
    if deltas:
        sample = mean(deltas)
    else:
        sample = 0
    final_dur = int(round((times_us[-1] - start_time) + sample))
    seg_durations.append(final_dur)
    seg_levels.append(cur_lvl)
    return seg_durations, seg_levels

def durations_to_c_array(durations, seg_levels):
    # Ensure array starts with a MARK (level 1). If first seg is SPACE (0), insert a zero-length MARK
    durations_out = durations[:]
    levels_out = seg_levels[:]
    if levels_out and levels_out[0] == 0:
        durations_out.insert(0, 0)
        levels_out.insert(0, 1)
    # produce array of durations in order MARK, SPACE, MARK, ...
    # double-check alternating pattern
    # return only durations
    return durations_out, levels_out

def detect_carrier_from_edges(times, levels):
    # build list of edge times (when level changes)
    edges = [t for i, t in enumerate(times) if i==0 or levels[i]!=levels[i-1]]
    # look for short alternating edges (<200 us) that repeat -> suggests carrier
    short_intervals = []
    for i in range(1, len(edges)):
        dt = edges[i] - edges[i-1]
        if dt > 0 and dt < 200:  # <200us considered possible carrier half-period
            short_intervals.append(dt)
    if not short_intervals:
        return None
    avg = mean(short_intervals)
    # carrier period approx 2*avg if these are half periods; but depending how edges recorded could be full period
    # best estimate: freq = 1/(avg) if we observed full-period edges, else 1/(2*avg)
    freq1 = 1e6 / avg  # Hz
    freq2 = 1e6 / (2*avg)
    # choose the one in typical IR range 30-60kHz
    if 30000 <= freq1 <= 60000:
        return freq1
    if 30000 <= freq2 <= 60000:
        return freq2
    # otherwise return the closer
    return freq1

def make_c_declaration(durations):
    arr = ", ".join(str(int(x)) for x in durations)
    return f"const uint16_t ir_signal[] = {{ {arr} }};\nconst size_t ir_len = {len(durations)};"

def main():
    if len(sys.argv) < 2:
        print("Uso: python csv_to_ir_array_from_samples.py archivo.csv")
        sys.exit(1)
    fn = sys.argv[1]
    times, levels = read_samples(fn)
    # times probably in microseconds if header says microseconds; but code handles seconds->us conversion
    seg_durations, seg_levels = group_segments(times, levels)
    durations, levels_out = durations_to_c_array(seg_durations, seg_levels)
    # detect carrier
    # convert times to microseconds for detection if necessary
    times_us = times[:]
    if max(times_us) < 1e3:
        times_us = [t * 1_000_000 for t in times_us]
    carrier = detect_carrier_from_edges(times_us, levels)
    # print summary
    print("=== RESULTADOS ===")
    print(f"Muestras totales: {len(times)}")
    print(f"Segmentos (nivel constante): {len(seg_durations)}")
    total = sum(durations)
    print(f"Duración total aproximada: {total} µs ({total/1000.0:.3f} ms)")
    print(f"Primera muestra nivel: {levels[0]}")
    if levels_out[0] == 1:
        print("Array empezará por MARK (ON).")
    else:
        print("Array empezará por SPACE (el script insertó un MARK de 0 para ajustar).")
    if carrier:
        print(f"Frecuencia de portadora estimada ≈ {carrier/1000.0:.2f} kHz")
    else:
        print("No se detectó portadora con claridad.")
    print("\n=== ARRAY C ===")
    print(make_c_declaration(durations))
    print("\n(Notas: los valores están en microsegundos, y alternan MARK/SPACE/ MARK / SPACE ...)\n")

if __name__ == "__main__":
    main()
