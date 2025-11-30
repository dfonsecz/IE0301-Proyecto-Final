#!/usr/bin/env python3
"""
Visualizador de estadisticas de tegrastats
Universidad de Costa Rica - IE0301

Uso:
    python3 plot_stats.py                    # Procesa todos los logs en stats/
    python3 plot_stats.py archivo.log        # Procesa un archivo especifico
    python3 plot_stats.py --all              # Genera graficos combinados
"""

import re
import sys
import glob
from pathlib import Path
import matplotlib.pyplot as plt
from datetime import datetime, timedelta

def parse_tegrastats_line(line):
    """Parse una línea de tegrastats y extrae métricas"""
    data = {}
    
    # CPU usage: [CPU%@MHz]
    # Ejemplo: CPU [12%@1479,16%@1479,14%@1479,13%@1479]
    cpu_match = re.search(r'CPU \[([\d%@,]+)\]', line)
    if cpu_match:
        cpu_values = cpu_match.group(1).split(',')
        cpu_usage = []
        for val in cpu_values:
            percent = re.search(r'(\d+)%', val)
            if percent:
                cpu_usage.append(int(percent.group(1)))
        if cpu_usage:
            data['cpu_avg'] = sum(cpu_usage) / len(cpu_usage)
            data['cpu_cores'] = cpu_usage
    
    # GPU usage: GR3D_FREQ X% o GR3D_FREQ X%@Y
    gpu_match = re.search(r'GR3D_FREQ (\d+)%(?:@(\d+))?', line)
    if gpu_match:
        data['gpu_usage'] = int(gpu_match.group(1))
        if gpu_match.group(2):
            data['gpu_freq'] = int(gpu_match.group(2))
        else:
            data['gpu_freq'] = None   # A veces Jetson no reporta frecuencia
    
    # RAM usage: RAM XXXX/YYYYMB
    # Ejemplo: RAM 2847/3964MB
    ram_match = re.search(r'RAM (\d+)/(\d+)MB', line)
    if ram_match:
        used = int(ram_match.group(1))
        total = int(ram_match.group(2))
        data['ram_used'] = used
        data['ram_total'] = total
        data['ram_percent'] = (used / total) * 100 if total > 0 else 0
    
    # Temperature: CPU@XXC
    temp_match = re.search(r'CPU@([\d.]+)C', line)
    if temp_match:
        data['temp_cpu'] = float(temp_match.group(1))
        
    return data

def parse_tegrastats_file(filepath):
    """Parse un archivo completo de tegrastats"""
    data = {
        'time': [],
        'cpu_avg': [],
        'gpu_usage': [],
        'ram_percent': [],
        'temp_cpu': []
    }
    
    start_time = datetime.now()
    
    with open(filepath, 'r') as f:
        for i, line in enumerate(f):
            if line.startswith('#'):
                continue
            
            parsed = parse_tegrastats_line(line)
            if parsed:
                # Timestamp relativo (asumiendo 500ms entre muestras)
                data['time'].append(start_time + timedelta(milliseconds=i*500))
                
                data['cpu_avg'].append(parsed.get('cpu_avg', 0))
                data['gpu_usage'].append(parsed.get('gpu_usage', 0))
                data['ram_percent'].append(parsed.get('ram_percent', 0))
                data['temp_cpu'].append(parsed.get('temp_cpu', 0))
    
    return data

def plot_single_video(filepath, output_dir='plots'):
    """Genera graficos para un solo video"""
    Path(output_dir).mkdir(exist_ok=True)
    
    video_name = Path(filepath).stem.replace('_stats', '')
    print(f"Procesando: {video_name}")
    
    data = parse_tegrastats_file(filepath)
    
    if not data['time']:
        print(f"WARNING: No se encontraron datos en {filepath}")
        return
    
    # Crear figura con subplots
    fig, axes = plt.subplots(2, 2, figsize=(15, 10))
    fig.suptitle(f'Estadísticas de Recursos - {video_name}', fontsize=16, fontweight='bold')
    
    # Convertir tiempo a segundos relativos
    start = data['time'][0]
    time_seconds = [(t - start).total_seconds() for t in data['time']]
    
    # CPU Usage
    ax = axes[0, 0]
    ax.plot(time_seconds, data['cpu_avg'], 'b-', linewidth=2, label='CPU Promedio')
    ax.fill_between(time_seconds, data['cpu_avg'], alpha=0.3)
    ax.set_ylabel('CPU Usage (%)', fontsize=12)
    ax.set_xlabel('Tiempo (s)', fontsize=12)
    ax.set_title('Uso de CPU', fontweight='bold')
    ax.grid(True, alpha=0.3)
    ax.set_ylim([0, 100])
    ax.legend()
    
    # GPU Usage
    ax = axes[0, 1]
    ax.plot(time_seconds, data['gpu_usage'], 'g-', linewidth=2, label='GPU')
    ax.fill_between(time_seconds, data['gpu_usage'], alpha=0.3, color='green')
    ax.set_ylabel('GPU Usage (%)', fontsize=12)
    ax.set_xlabel('Tiempo (s)', fontsize=12)
    ax.set_title('Uso de GPU', fontweight='bold')
    ax.grid(True, alpha=0.3)
    ax.set_ylim([0, 100])
    ax.legend()
    
    # RAM Usage
    ax = axes[1, 0]
    ax.plot(time_seconds, data['ram_percent'], 'r-', linewidth=2, label='RAM')
    ax.fill_between(time_seconds, data['ram_percent'], alpha=0.3, color='red')
    ax.set_ylabel('RAM Usage (%)', fontsize=12)
    ax.set_xlabel('Tiempo (s)', fontsize=12)
    ax.set_title('Uso de RAM', fontweight='bold')
    ax.grid(True, alpha=0.3)
    ax.set_ylim([0, 100])
    ax.legend()
    
    # Temperature
    ax = axes[1, 1]
    ax.plot(time_seconds, data['temp_cpu'], 'orange', linewidth=2, label='Temperatura CPU')
    ax.set_ylabel('Temperatura (°C)', fontsize=12, color='orange')
    ax.tick_params(axis='y', labelcolor='orange')
    ax.set_xlabel('Tiempo (s)', fontsize=12)
    ax.set_title('Temperatura CPU', fontweight='bold')
    ax.grid(True, alpha=0.3)
    ax.legend(loc='upper left')
    
    plt.tight_layout()
    
    # Guardar gráfico
    output_file = f"{output_dir}/{video_name}_stats.png"
    plt.savefig(output_file, dpi=300, bbox_inches='tight')
    print(f"[OK] Grafico guardado: {output_file}")
    
    # Imprimir estadísticas
    print(f"\nEstadisticas de {video_name}:")
    print(f"   CPU Promedio:  {sum(data['cpu_avg'])/len(data['cpu_avg']):.1f}%")
    print(f"   CPU Maximo:    {max(data['cpu_avg']):.1f}%")
    print(f"   GPU Promedio:  {sum(data['gpu_usage'])/len(data['gpu_usage']):.1f}%")
    print(f"   GPU Maximo:    {max(data['gpu_usage']):.1f}%")
    print(f"   RAM Promedio:  {sum(data['ram_percent'])/len(data['ram_percent']):.1f}%")
    print(f"   Temp Promedio: {sum(data['temp_cpu'])/len(data['temp_cpu']):.1f}C")
    print(f"   Temp Maxima:   {max(data['temp_cpu']):.1f}C")
    print()
    
    plt.close()

def plot_all_combined(stats_dir='stats', output_dir='plots'):
    """Genera grafico combinado de todos los videos"""
    Path(output_dir).mkdir(exist_ok=True)
    
    files = sorted(glob.glob(f"{stats_dir}/*_stats.log"))
    
    if not files:
        print(f"WARNING: No se encontraron archivos en {stats_dir}/")
        return
    
    fig, axes = plt.subplots(2, 2, figsize=(15, 10))
    fig.suptitle('Comparación de Recursos - Todos los Videos', fontsize=16, fontweight='bold')
    
    colors = plt.cm.tab10(range(len(files)))
    
    for i, filepath in enumerate(files):
        video_name = Path(filepath).stem.replace('_stats', '')
        data = parse_tegrastats_file(filepath)
        
        if not data['time']:
            continue
        
        start = data['time'][0]
        time_seconds = [(t - start).total_seconds() for t in data['time']]
        
        # CPU
        axes[0, 0].plot(time_seconds, data['cpu_avg'], 
                       color=colors[i], linewidth=2, label=video_name, alpha=0.7)
        
        # GPU
        axes[0, 1].plot(time_seconds, data['gpu_usage'], 
                       color=colors[i], linewidth=2, label=video_name, alpha=0.7)
        
        # RAM
        axes[1, 0].plot(time_seconds, data['ram_percent'], 
                       color=colors[i], linewidth=2, label=video_name, alpha=0.7)
        
        # Temperature
        axes[1, 1].plot(time_seconds, data['temp_cpu'], 
                       color=colors[i], linewidth=2, label=video_name, alpha=0.7)
    
    # Configurar ejes
    titles = ['CPU Usage (%)', 'GPU Usage (%)', 'RAM Usage (%)', 'Temperatura (°C)']
    for ax, title in zip(axes.flat, titles):
        ax.set_title(title, fontweight='bold')
        ax.set_xlabel('Tiempo (s)', fontsize=10)
        ax.set_ylabel(title, fontsize=10)
        ax.grid(True, alpha=0.3)
        ax.legend(fontsize=8, loc='best')
    
    plt.tight_layout()
    output_file = f"{output_dir}/combined_stats.png"
    plt.savefig(output_file, dpi=300, bbox_inches='tight')
    print(f"[OK] Grafico combinado guardado: {output_file}")
    plt.close()

def main():
    """Funcion principal"""
    print("=" * 60)
    print("  Visualizador de Estadisticas - ROI Surveillance")
    print("=" * 60)
    print()
    
    if len(sys.argv) > 1:
        if sys.argv[1] == '--all':
            plot_all_combined()
        else:
            plot_single_video(sys.argv[1])
    else:
        # Procesar todos los archivos en stats/
        files = glob.glob("stats/*_stats.log")
        if not files:
            print("WARNING: No se encontraron archivos en stats/")
            print("Ejecuta primero: ./quick_test.sh")
            return
        
        print(f"Encontrados {len(files)} archivos de estadisticas")
        print()
        
        for filepath in sorted(files):
            plot_single_video(filepath)
        
        # Generar grafico combinado
        print("Generando grafico combinado...")
        plot_all_combined()
    
    print()
    print("=" * 60)
    print("Procesamiento completo!")
    print("Graficos guardados en: plots/")
    print("=" * 60)

if __name__ == '__main__':
    try:
        import matplotlib
        main()
    except ImportError:
        print("ERROR: matplotlib no esta instalado")
        print("Instalalo con: sudo apt-get install python3-matplotlib")
        print("O con pip: pip3 install matplotlib")
        sys.exit(1)
