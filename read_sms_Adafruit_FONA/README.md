Some notes to take into consideration:
 - The TinyGSM library doesn't work to receive SMS, because it wasn't updated yet (the author himself said it on a git forum)
 - It was used the Adafruit_FONA library
 - The code for sending an SMS is not working with this library in that specific place of the code (line 250), but it must be easy to change to the TinyGSM library.
 - Tests: we must test this code with the sleep cicles of the parallel_tasks routines.