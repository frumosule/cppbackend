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
        try:
            process.wait(timeout=5)
        except subprocess.TimeoutExpired:
            process.kill()

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
    
    time.sleep(2)
    
    # Check if server is running
    if server.poll() is not None:
        print("Server failed to start")
        return
    
    perf_command = f'perf record -g -p {server.pid} -o perf.data --call-graph dwarf'
    perf_run = run(perf_command)
    
    make_shots()
    
    # Stop perf recording
    if perf_run.poll() is None:
        perf_run.send_signal(signal.SIGINT)
        perf_run.wait(timeout=10)
    
    # Stop server
    stop(server, wait=True)
    
    # Check if perf.data was created
    if not os.path.exists('perf.data'):
        print("perf.data was not created")
        return
    
    print("Processing perf data...")
    
    # Ensure FlameGraph tools exist
    if not os.path.exists('./FlameGraph/stackcollapse-perf.pl'):
        print("FlameGraph tools not found")
        return
    
    try:
        p1 = subprocess.Popen([
            'perf', 'script', 
            '-i', 'perf.data',
            '--demangle'
        ], stdout=subprocess.PIPE, stderr=subprocess.PIPE)

        p2 = subprocess.Popen([
            './FlameGraph/stackcollapse-perf.pl', 
            '--demangle'
        ], stdin=p1.stdout, stdout=subprocess.PIPE, stderr=subprocess.PIPE)

        with open('graph.svg', 'w', encoding='utf-8') as out_file:
            p3 = subprocess.Popen([
                './FlameGraph/flamegraph.pl',
                '--title', 'CPU Flame Graph',
                '--width', '1600',
                '--colors', 'java'
            ], stdin=p2.stdout, stdout=out_file, stderr=subprocess.PIPE)

        p1.stdout.close()
        p2.stdout.close()
        
        # Wait for all processes to complete
        p1.wait(timeout=30)
        p2.wait(timeout=30)
        p3.wait(timeout=30)
        
        # Check for errors
        if p1.returncode != 0:
            stderr = p1.stderr.read().decode() if p1.stderr else ''
            print(f"perf script failed: {stderr}")
        
        if p2.returncode != 0:
            stderr = p2.stderr.read().decode() if p2.stderr else ''
            print(f"stackcollapse failed: {stderr}")
        
        if p3.returncode != 0:
            stderr = p3.stderr.read().decode() if p3.stderr else ''
            print(f"flamegraph failed: {stderr}")

        if os.path.exists('graph.svg'):
            print('Flame graph created successfully')
        else:
            print('Failed to create flame graph')

    except subprocess.TimeoutExpired:
        print("Processing timed out")
    except Exception as e:
        print(f"Error during processing: {e}")

    print('Job done')
