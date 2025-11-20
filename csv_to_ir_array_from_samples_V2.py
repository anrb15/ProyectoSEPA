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
    time_col = None
    level_col = None
    for i, h in enumerate(header):
        lh = h.lower()
        if "micro" in lh or "time" in lh:
            time_col = i
        if "logic" in lh or "channel" in lh or lh in ("0","1","d0","d1"):
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
        if time_col is None or level_col is None:
            first_row = None
            for row in reader:
                if not row:
                    continue
                first_row = row
                break
            if first_row is None:
                raise SystemExit("No hay filas de datos en el CSV.")
            if len(first_row) >= 2 and safe_float(first_row[0]) is not None and first_row[1] in ("0","1"):
                time_col = 0
                level_col = 1
                times.append(float(first_row[time_col]))
                levels.append(int(first_row[level_col]))
            else:
                f.seek(0)
                reader = csv.reader(f)
                for row in reader:
                    if not row:
                        continue
                    val = row[0].strip()
                    if val in ("0","1"):
                        levels.append(int(val))
                raise SystemExit("CSV solo contiene niveles sin timestamps.")
        else:
            for row in reader:
                if len(row) <= max(time_col, level_col):
                    continue
                t = safe_float(row[time_col])
                if t is None:
                    continue
                lvl = row[level_col].strip()
                if lvl not in ("0","1"):
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
    times_us = times[:]
    if max(times_us) < 1e3:
        times_us = [t * 1_000_000.0 for t in times_us]
    seg_durations = []
    seg_levels = []
    start_time = times_us[0]
    cur_lvl = levels[0]
    for i in range(1, len(times_us)):
        if levels[i] != cur_lvl:
            dur = int(round(times_us[i] - start_time))
            seg_durations.append(dur)
            seg_levels.append(cur_lvl)
            start_time = times_us[i]
            cur_lvl = levels[i]
    deltas = [times_us[i+1]-times_us[i] for i in range(len(times_us)-1)]
    sample = mean(deltas) if deltas else 0
    final_dur = int(round((times_us[-1] - start_time) + sample))
    seg_durations.append(final_dur)
    seg_levels.append(cur_lvl)
    return seg_durations, seg_levels

def durations_to_c_array(durations, seg_levels):
    durations_out = durations[:]
    levels_out = seg_levels[:]
    if levels_out and levels_out[0] == 0:
        durations_out.insert(0, 0)
        levels_out.insert(0, 1)
    return durations_out, levels_out

# === NUEVA FUNCIÓN PARA ELIMINAR REPETICIONES IR ===
def clean_repeats(durations, levels, gap_threshold=20000):
    """
    Recorta la señal en cuanto aparece un GAP muy largo (>20 ms por defecto),
    lo cual indica el fin del primer paquete IR.
    """
    cleaned_durations = []
    cleaned_levels = []
    for d, lvl in zip(durations, levels):
        cleaned_durations.append(d)
        cleaned_levels.append(lvl)
        if d > gap_threshold:
            break
    return cleaned_durations, cleaned_levels

def detect_carrier_from_edges(times, levels):
    edges = [t for i, t in enumerate(times) if i==0 or levels[i]!=levels[i-1]]
    short_intervals = []
    for i in range(1, len(edges)):
        dt = edges[i] - edges[i-1]
        if dt > 0 and dt < 200:
            short_intervals.append(dt)
    if not short_intervals:
        return None
    avg = mean(short_intervals)
    freq1 = 1e6 / avg
    freq2 = 1e6 / (2*avg)
    if 30000 <= freq1 <= 60000:
        return freq1
    if 30000 <= freq2 <= 60000:
        return freq2
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
    seg_durations, seg_levels = group_segments(times, levels)
    durations, levels_out = durations_to_c_array(seg_durations, seg_levels)

    # === LIMPIEZA AUTOMÁTICA DE REPETICIONES ===
    durations, levels_out = clean_repeats(durations, levels_out, gap_threshold=20000)

    times_us = times[:]
    if max(times_us) < 1e3:
        times_us = [t * 1_000_000 for t in times_us]
    carrier = detect_carrier_from_edges(times_us, levels)

    print("=== RESULTADOS ===")
    print(f"Muestras totales: {len(times)}")
    print(f"Segmentos (nivel constante): {len(seg_durations)}")
    total = sum(durations)
    print(f"Duración total después de limpieza: {total} µs ({total/1000.0:.3f} ms)")
    print(f"Primera muestra nivel: {levels[0]}")
    if levels_out[0] == 1:
        print("Array empezará por MARK (ON).")
    else:
        print("Array empezará por SPACE.")
    if carrier:
        print(f"Frecuencia de portadora estimada ≈ {carrier/1000.0:.2f} kHz")
    else:
        print("No se detectó portadora clara.")

    print("\n=== ARRAY C ===")
    print(make_c_declaration(durations))
    print("\n(Notas: valores en microsegundos, alternan MARK / SPACE ...)\n")

if __name__ == "__main__":
    main()
