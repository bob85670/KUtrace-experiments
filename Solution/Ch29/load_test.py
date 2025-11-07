import random, time, sys, heapq

random.seed(42)

def job(skewed):
    return 1000 if skewed and random.random()<0.01 else random.uniform(10,20)

def real_server(rps, sec=4, skewed=False):
    interval = 1.0/rps
    start = time.time()
    heap = []                # (finish_time, lat)
    done = []
    next_send = start
    while time.time() < start + sec or heap:
        now = time.time()
        # Send new
        if now < start + sec and now >= next_send:
            lat = job(skewed)
            heapq.heappush(heap, (now + lat/1000, lat))
            next_send += interval
        # Finish ready
        while heap and heap[0][0] <= now:
            _, lat = heapq.heappop(heap)
            done.append(lat)
        # Sleep to next event
        next_t = min(next_send, heap[0][0] if heap else float('inf'))
        if next_t > now:
            time.sleep(min(next_t - now, 0.02))
    return done

def p90(lats):
    s = sorted(lats)
    return s[int(0.9*len(s))]

def find(goal, skewed):
    lo, hi = 10, 6000
    best = 0
    for i in range(16):
        mid = (lo+hi)/2
        lats = real_server(mid, 4, skewed)
        avg = sum(lats)/len(lats)
        ratio = p90(lats)/avg
        slow = sum(1 for x in lats if x>100)
        print(f"\r[{i+1:2}/16] {mid:4.0f} RPS → {ratio:.2f}x  slow:{slow}", end="")
        sys.stdout.flush()
        if ratio <= goal:
            best = mid; lo = mid
        else:
            hi = mid
    print()
    return round(best)

print("Uniform →", find(1.50, False), "RPS")
print("Skewed  →", find(1.75, True), "RPS")