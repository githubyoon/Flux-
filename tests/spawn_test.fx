function worker(string msg) {
    printf("Thread says: {msg}");
}
function main() {
    printf("Main starts...");
    spawn(worker, "Hello from spawned thread!");
    spawn(worker, "Another thread message!");
    printf("Main waiting for threads...");
    time.sleep(100);
    printf("Main done.");
}
