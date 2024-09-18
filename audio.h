#define AUDIO_PIN A5

#define note_t struct note
struct note {
  // frequency in hz
  uint16_t freq;
  // duration in ms
  uint16_t duration;
};

#define MAX_NOTES 5
uint16_t note_count = 0;
uint16_t note_ind = 0;
note_t NOTES[MAX_NOTES];

void addNote (uint16_t freq, uint16_t duration) {
  if (note_count == MAX_NOTES) {
    return;
  }
  NOTES[note_count].freq = freq;
  NOTES[note_count].duration = duration;
  note_count += 1;
}

void clearNotes () {
  note_count = 0;
  note_ind = 0;
}

bool noteToggle = 0;
uint32_t noteToggles = 0;

// https://www.instructables.com/Arduino-Timer-Interrupts/
void playNote () {
  cli();

  if (note_ind >= note_count) {
    // disable interrupt
    TIMSK1 &= ~(1 << OCIE1A);
    return;
  }

  TCCR1A = 0; // set entire TCCR1A register to 0
  TCCR1B = 0; // same for TCCR1B
  TCNT1  = 0; //initialize counter value to 0
  // turn on CTC mode
  TCCR1B |= (1 << WGM12);
  // Set CS11 for 8 prescaler
  TCCR1B |= (1 << CS11);

  uint16_t freq = NOTES[note_ind].freq * 2;
  uint16_t duration = NOTES[note_ind].duration;
  OCR1A = (uint32_t) 16000000 / 8 / freq - 1;
  noteToggles = duration * freq / 1000;

  // enable timer compare interrupt
  TIMSK1 |= (1 << OCIE1A);

  note_ind += 1;

  sei();
}

ISR(TIMER1_COMPA_vect){
  digitalWrite(AUDIO_PIN, noteToggle);
  noteToggle = !noteToggle;
  noteToggles -= 1;
  if (noteToggles == 0) {
    playNote();
  }
}