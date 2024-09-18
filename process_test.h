#pragma once

void process_a (process_t* p, time_t deadline) {
  // if (timeUntil(deadline) < 110) {
  //   return;
  // }
  time_t now = my_millis();
  p->next_run = now + 20;
  #ifdef process_test_debug
  // Serial.print(now);
  // Serial.println(": a");
  #endif
  // delay(100);
}

void process_b (process_t* p, time_t deadline) {
  time_t now = my_millis();
  #ifdef process_test_debug
  Serial.print(now);
  Serial.println(": b");
  #endif
  p->next_run += 250;
}

void process_c (process_t* p, time_t deadline) {
  time_t now = my_millis();
  #ifdef process_test_debug
  Serial.print(now);
  Serial.println(": c");
  #endif
  p->next_run += 500;
}

void processesTest () {
  time_t now = my_millis();
  #ifdef process_test_debug
    Serial.print("Testing processes, now is ");
    Serial.println(now);
  #endif

  createProcessWithName(process_a, now, DEADLINE, 1, 0, "tsta");
  createProcessWithName(process_b, now + 30, FIXED, 1, 0, "tstb");
  createProcessWithName(process_c, now + 60, FIXED, 1, 0, "tstc");
}