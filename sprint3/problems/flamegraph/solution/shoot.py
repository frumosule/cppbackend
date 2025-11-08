import argparse
import subprocess
import time
import random
import shlex
import os

RANDOM_LIMIT = 1000
SEED = 123456789
random.seed(SEED)

AMMUNITION = [
    'http://localhost:8080/api/v1/maps/map1',
    'http://localhost:8080/api/v1/maps'
]

SHOOT_COUNT = 100
COOLDOWN = 0.1

def start_server():
    parser = argparse.ArgumentParser()
    parser.add_argument('server', type=str)
    return parser.parse_args().server

def run(command, output=None):
    process = subprocess.Popen(shlex.split(command), stdout=output, stderr=subprocess.DEVNULL)
    return process

def stop(process, wait=False):
    if process.poll() is None and wait:
        process.wait()
    process.terminate()

def shoot(ammo):
    hit = run(f'curl -s "{ammo}"', output=subprocess.DEVNULL)
    time.sleep(COOLDOWN)
    stop(hit, wait=True)

def make_shots():
    for _ in range(SHOOT_COUNT):
        ammo_number = random.randrange(RANDOM_LIMIT) % len(AMMUNITION)
        shoot(AMMUNITION[ammo_number])
    print('Shooting complete')

def main():
    server_command = start_server()
    server = run(server_command)
    
    # Даем серверу время на запуск
    time.sleep(2)
    
    # Запускаем perf record с правильными параметрами
    perf_command = f'sudo perf record -g -p {server.pid} -o perf.data --call-graph dwarf'
    perf_run = run(perf_command)
    
    make_shots()
    
    # Останавливаем perf
    stop(perf_run, wait=True)
    # Останавливаем сервер
    stop(server, wait=True)
    
    time.sleep(1)

    # Обрабатываем результаты perf
    print("Processing perf data...")
    
    # perf script с деманглингом символов
    p1 = subprocess.Popen([
        'sudo', 'perf', 'script', 
        '-i', 'perf.data',
        '--demangle'
    ], stdout=subprocess.PIPE)

    # stackcollapse
    p2 = subprocess.Popen([
        './FlameGraph/stackcollapse-perf.pl', 
        '--demangle'
    ], stdin=p1.stdout, stdout=subprocess.PIPE)

    # flamegraph
    with open('graph.svg', 'w', encoding='utf-8') as out_file:
        p3 = subprocess.Popen([
            './FlameGraph/flamegraph.pl',
            '--title', 'CPU Flame Graph',
            '--width', '1600',
            '--colors', 'java'
        ], stdin=p2.stdout, stdout=out_file)

    p1.stdout.close()
    p2.stdout.close()
    p3.wait()

    print('Job done')

if __name__ == '__main__':
    main()