import argparse
import subprocess
import time
import random
import shlex

RANDOM_LIMIT = 1000
SEED = 123456789
random.seed(SEED)

AMMUNITION = [
    'localhost:8080/api/v1/maps/map1',
    'localhost:8080/api/v1/maps'
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
    hit = run('curl ' + ammo, output=subprocess.DEVNULL)
    time.sleep(COOLDOWN)
    stop(hit, wait=True)

def make_shots():
    for _ in range(SHOOT_COUNT):
        ammo_number = random.randrange(RANDOM_LIMIT) % len(AMMUNITION)
        shoot(AMMUNITION[ammo_number])
    print('Shooting complete')

server = run(start_server())
perf_run  = subprocess.Popen(shlex.split(f' sudo perf record -g -p {server.pid} -o ./perf.data'))
make_shots()

stop(perf_run, wait=True)
time.sleep(2) 

stop(server)
time.sleep(1)

# perf script
p1 = subprocess.Popen(['sudo', 'perf', 'script', '-f', '-i', 'perf.data'], stdout=subprocess.PIPE)

# stackcollapse
p2 = subprocess.Popen(['./FlameGraph/stackcollapse-perf.pl'], stdin=p1.stdout, stdout=subprocess.PIPE)

# flamegraph -> graph.svg
with open('graph.svg', 'w', encoding='utf-8') as out_file:
    p3 = subprocess.Popen(['./FlameGraph/flamegraph.pl'], stdin=p2.stdout, stdout=out_file)

# Close unused pipes
p1.stdout.close()
p2.stdout.close()

# Wait final process
p3.wait()

print('Job done')