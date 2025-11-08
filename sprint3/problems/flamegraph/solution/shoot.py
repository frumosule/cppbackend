import argparse
import subprocess
import time
import random
import shlex
import os
import signal

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
    if process.poll() is None:
        if wait:
            process.wait()
        process.terminate()

def shoot(ammo):
    # Используем полный URL с протоколом
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
    
    # Запускаем сервер
    server = run(server_command, output=subprocess.DEVNULL)
    time.sleep(2)  # Даем серверу время на запуск
    
    # Проверяем, что сервер запустился
    if server.poll() is not None:
        print("Server failed to start!")
        return
    
    print(f"Server started with PID: {server.pid}")
    
    # Запускаем perf record с явным указанием выходного файла
    perf_command = f'perf record -g -p {server.pid} -o perf.data --call-graph dwarf'
    perf_run = run(perf_command)
    
    # Генерируем нагрузку
    make_shots()
    
    # Останавливаем perf record (отправляем SIGINT)
    perf_run.send_signal(signal.SIGINT)
    perf_run.wait()
    
    # Останавливаем сервер
    stop(server, wait=True)
    
    time.sleep(1)
    
    # Проверяем, что perf.data создан
    if not os.path.exists('perf.data'):
        print("perf.data not found!")
        return
    
    print("Processing perf data...")
    
    # Создаем флеймграф
    try:
        # perf script -> stackcollapse -> flamegraph
        p1 = subprocess.Popen([
            'perf', 'script', 
            '-i', 'perf.data'
        ], stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        
        p2 = subprocess.Popen([
            './FlameGraph/stackcollapse-perf.pl'
        ], stdin=p1.stdout, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        
        with open('graph.svg', 'w', encoding='utf-8') as out_file:
            p3 = subprocess.Popen([
                './FlameGraph/flamegraph.pl',
                '--title', 'CPU Flame Graph',
                '--width', '1600',
                '--colors', 'java'
            ], stdin=p2.stdout, stdout=out_file, stderr=subprocess.PIPE)
        
        # Закрываем неиспользуемые pipe'ы
        p1.stdout.close()
        p2.stdout.close()
        
        # Ждем завершения всех процессов
        p1.wait()
        p2.wait()
        p3.wait()
        
        # Проверяем ошибки
        if p1.returncode != 0:
            print(f"perf script error: {p1.stderr.read().decode()}")
        if p2.returncode != 0:
            print(f"stackcollapse error: {p2.stderr.read().decode()}")
        if p3.returncode != 0:
            print(f"flamegraph error: {p3.stderr.read().decode()}")
            
    except Exception as e:
        print(f"Error creating flamegraph: {e}")
        return
    
    # Проверяем содержимое graph.svg
    if os.path.exists('graph.svg'):
        with open('graph.svg', 'r') as f:
            content = f.read()
            if 'http_handler::RequestHandler' in content:
                print("✓ Success: http_handler::RequestHandler found in flamegraph")
            else:
                print("✗ Warning: http_handler::RequestHandler not found in flamegraph")
                print("This might indicate that:")
                print("1. Server binary was built without debug symbols")
                print("2. Function names are different")
                print("3. perf didn't capture the samples properly")
    else:
        print("graph.svg not created!")
    
    print('Job done')