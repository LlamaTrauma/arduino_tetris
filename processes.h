#pragma once

#include <string.h>

#define time_t uint16_t

inline time_t my_millis () {
  // return millis() >> 4;
  return millis();
}

// #define PROCESS_DEBUG

inline int16_t subtractTimes (time_t a, time_t b) {
  return a - b;
}

inline int16_t timeUntil (time_t a) {
  return a - ((time_t) my_millis());
}

// Fixed - must run immediately at next_time
// Deadline - must at or before next_time
// Whenever - Can run whenever there's downtime
enum ProcessType {FIXED, DEADLINE, WHENEVER};

#define process_t struct process
struct process {
  // Process type
  ProcessType type;
  // When did this last run - used to prioritize floating/free processes
  time_t last_run;
  // When should it next run - used for fixed/floating processes
  time_t next_run;
  // How important is this - used to schedule non-fixed tasks before each other
  uint8_t priority;
  // Used for selectively clearing processes
  uint8_t level;
  // Where is this in the process list - updates when it gets shuffled around
  uint8_t ind;
  // What to do when this runs
  // Pass in a pointer to its process and the time until the next process is scheduled to run
  void (*event) (process_t*, time_t);
  // Name - hopefully I never use this for more than debugging
  char name[5];
  // Does this remain on the queue after a run
  // Defaults to true in the "constructor", must be set to false explicitly
  bool repeat;
};

#define MAX_PROCESSES 5
uint8_t num_processes = 0;
process_t* PROCESSES[MAX_PROCESSES];

// For checking if a process deletes itself while running
process_t* running_process;
bool running_process_deleted;

int compareProcesses (const void* a, const void* b) {
  // Serial.print("a is ");
  // Serial.println((uint16_t) a);
  // Serial.print("b is ");
  // Serial.println((uint16_t) b);
  time_t a_next = ((process_t*) a)->next_run;
  time_t b_next = ((process_t*) b)->next_run;
  if (a_next == b_next) {
    return 0;
  }
  int16_t diff = a_next - b_next;
  return diff;
}

// Doesn't matter for fixed processes, just don't call this for them
// This is a bundle of heuristics that I hope makes logical sense
uint16_t scoreProcess (process_t* p, time_t now) {
  uint16_t score = 0x55;

  score *= p->priority;

  time_t since = max(1, subtractTimes(now, p->last_run) + 1);    
  score /= since;

  if (p->type == DEADLINE) {
    time_t remaining = min(0x50, subtractTimes(p->next_run, now));
    score *= remaining;
  }

  return score;
}

void insertProcess (process_t* p) {
  uint8_t ind = 0;
  process_t** at = &PROCESSES[0];
  while (ind < num_processes && compareProcesses(p, *at) > 0) {
    ind ++;
    at ++;
  }
  for (uint8_t i = num_processes; i > ind; i --) {
    PROCESSES[i] = PROCESSES[i - 1];
    PROCESSES[i]->ind = i;
  }
  PROCESSES[ind] = p;
  p->ind = ind;
  num_processes += 1;
  #ifdef PROCESS_DEBUG
    Serial.print("inserting process ");
    Serial.print(p->name);
    Serial.print(" at index ");
    Serial.print(ind);
    Serial.print(", process count is now ");
    Serial.println(num_processes);
  #endif
}

inline void reorderProcess (process_t* p) {
  uint8_t p_ind = p->ind;
  while (p_ind < num_processes - 1 && compareProcesses(p, PROCESSES[p_ind + 1]) > 0) {
    PROCESSES[p_ind] = PROCESSES[p_ind + 1];
    PROCESSES[p_ind]->ind = p_ind;
    p_ind += 1;
  }
  while (p_ind > 0 && compareProcesses(p, PROCESSES[p_ind - 1]) < 0) {
    PROCESSES[p_ind] = PROCESSES[p_ind - 1];
    PROCESSES[p_ind]->ind = p_ind;
    p_ind -= 1;
  }
  PROCESSES[p_ind] = p;
  p->ind = p_ind;
}

process_t* createProcessWithName (void (*event) (uint16_t), time_t next_run, ProcessType type, uint8_t priority, uint8_t level, const char* name) {
  if (num_processes == MAX_PROCESSES) {
    return;
  }
  process_t* p = malloc(sizeof(process_t));
  p->type = type;
  p->next_run = next_run;
  p->last_run = my_millis();
  p->priority = priority;
  p->level = level;
  p->event = event;
  p->repeat = true;
  strcpy(p->name, name);
  if (type == FIXED) {
    insertProcess(p);
  } else if (type == DEADLINE) {
    insertProcess(p);
  } else if (type == WHENEVER) {
    PROCESSES[num_processes] = p;
    p->ind = num_processes;
    num_processes += 1;
  }
  return p;
}

void deleteProcess (process_t* p) {
  uint8_t ind = p->ind;
  #ifdef PROCESS_DEBUG
    Serial.print("Deleting process ");
    Serial.print(p->name);
    Serial.print(" with ind ");
    Serial.print(ind);
    Serial.print(", number of processes before delete is ");
    Serial.println(num_processes);
  #endif
  for(uint8_t i = ind; i < num_processes - 1; i ++) {
    PROCESSES[i] = PROCESSES[i+1];
    PROCESSES[i]->ind = i;
  }
  num_processes -= 1;
  if (p == running_process) {
    running_process_deleted = true;
    #ifdef PROCESS_DEBUG
    Serial.println("The running process deleted itself");
    #endif
  }
  free(p);
}

void clearProcesses (uint8_t level) {
  uint8_t ind = 0;
  while (ind < num_processes) {
    if (PROCESSES[ind]->level >= level) {
      deleteProcess(PROCESSES[ind]);
    } else {
      ind += 1;
    }
  }
}

inline void runProcess (process_t* p, time_t deadline) {
  #ifdef PROCESS_DEBUG
    Serial.print("running process ");
    Serial.println(p->name);
  #endif
  running_process = p;
  running_process_deleted = false;
  p->event(p, deadline);
  if (running_process_deleted) {
    return;
  }
  p->last_run = my_millis();
  if (!p->repeat) {
    deleteProcess(p);
  } 
  else {
    reorderProcess(p);
  }
  #ifdef PROCESS_DEBUG
    Serial.println("done running");
  #endif
}

void runProcesses () {
  if (num_processes == 0) {
    return;
  }

  time_t now = my_millis();

  #ifdef PROCESS_DEBUG
    Serial.println("--------------------------");
    Serial.println("SUMMARY");
    Serial.print("t ");
    Serial.println(now);
    Serial.print("n ");
    Serial.println(num_processes);
    for(uint8_t i = 0; i < num_processes; i ++) {
      process_t* p = PROCESSES[i];
      Serial.print(p->name);
      Serial.println(":");
      Serial.print("  type ");
      Serial.println(p->type);
      Serial.print("  ind ");
      Serial.println(p->ind);
      Serial.print("  next ");
      Serial.println(p->next_run);
    }
  #endif

  process_t* next_up = PROCESSES[0];
  // Is the process with the soonest deadline fixed?
  if (next_up->type == FIXED) {
    int16_t deadline = next_up->next_run;
    // If it's fixed, is that deadline very soon?
    if (subtractTimes(deadline, now) <= 1) {
      // spinlock until then if yes
      int16_t next_deadline = num_processes > 1 ? PROCESSES[1]->next_run : (now + 20);
      while (timeUntil(deadline) > 0);
      runProcess(next_up, next_deadline);
      return;
    }
  }

  // Otherwise, pick a floating process to run
  next_up = NULL;
  // Maybe this is smart, maybe it's stupid, but I'm trying not to let the deadline of
  // a more sporadic process sneak up because this keeps running the process with the
  // soonest deadline
  uint16_t next_up_score = UINT16_MAX;
  for (uint8_t i = 0; i < num_processes; i ++) {
    process_t* p = PROCESSES[i];
    if (p->type != FIXED) {
      uint16_t score = scoreProcess(next_up, now);
      if (score < next_up_score) {
        next_up = p;
        next_up_score = score;
      }
    }
  }
  if (!next_up) {
    // There are no floating processes
    return;
  }
  time_t deadline = now + 20;
  if (num_processes > 1) {
    deadline = subtractTimes(PROCESSES[!(next_up->ind)]->next_run, now);
  }
  runProcess(next_up, deadline);
}