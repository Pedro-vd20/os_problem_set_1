all: hyperloop_semaphore hyperloop_condition_var


hyperloop_semaphore: hyperloop_semaphore.c
	gcc -pthread hyperloop_semaphore.c -o hyperloop_semaphore

hyperloop_condition_var:
	gcc -pthread hyperloop_condition_var.c -o hyperloop_condition_var

clean:
	rm hyperloop_condition_var
	rm hyperloop_semaphore