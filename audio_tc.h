// #include <SPI.h>
// #include <SdFat.h>

// #define AUDIO_BUFFER_SIZE 1024
// #define FILL_SIZE 512
// #define AUDIO_INTERRUPT_TC TC5
// #define DAC A0

// uint16_t AUDIO_BUFFER[AUDIO_BUFFER_SIZE];
// uint16_t audio_buffer_loc = 0;
// uint16_t audio_buffer_end = 0;
// uint16_t next_buffer_update = 0;


// void circular_memcpy(void* dest, void* src, uint16_t dst_pos, uint16_t dst_size, uint16_t size) {
//   uint16_t s = dst_pos;
//   uint16_t m = s + size;
//   uint16_t e = m % dst_size;
//   if (m == e) {
//     memcpy(dest + s, src, size);
//   } else {
//     m = dst_size;
//     memcpy(dest + s, src, m - s);
//     memcpy(dest, src, e);
//   }
// }

// // https://forum.pjrc.com/index.php?threads/sdfat-read-write-example.65276/post-263415
// #define SD_CS_PIN 28
// #define SPI_CLOCK SD_SCK_MHZ(4)

// #if HAS_SDIO_CLASS
// #define SD_CONFIG SdioConfig(FIFO_SDIO)
// #elif ENABLE_DEDICATED_SPI
// #define SD_CONFIG SdSpiConfig(SD_CS_PIN, DEDICATED_SPI, SPI_CLOCK)
// #else  // HAS_SDIO_CLASS
// #define SD_CONFIG SdSpiConfig(SD_CS_PIN, SHARED_SPI, SPI_CLOCK)
// #endif  // HAS_SDIO_CLASS

// SdFat32 sd;

// struct audio_stream {
//   bool loop;
//   char fname[20];
//   uint32_t file_loc;
//   File32 file;
// };

// #define MAX_AUDIO_STREAMS 4
// uint8_t num_audio_streams = 0;

// audio_stream* audio_streams[MAX_AUDIO_STREAMS];

// void initAudio(){
//   initSd();
//   Serial.println("SD initialized");
//   pinMode(DAC, OUTPUT);
//   analogWriteResolution(10);
//   Serial.println("DAC set up");
//   audioTCConfigure();
//   Serial.println("Audio interrupt configured");
//   addTestStream();
//   Serial.println("Test stream added");
//   startAudioProcess();
//   Serial.println("Audio processes added");
// }

// void addAudioStream(char fname[], bool loop){
//   if (num_audio_streams == MAX_AUDIO_STREAMS) {
//     return;
//   }
//   audio_stream* s = (audio_stream*) malloc(sizeof(struct audio_stream));
//   s->loop = loop;
//   strcpy(s->fname, fname);
//   s->file_loc = 0;
//   s->file = sd.open(fname);
//   audio_streams[num_audio_streams++] = s;
// }

// void startAudioProcess () {
//   uint8_t now = millis();
//   next_buffer_update = now;
//   createProcess(updateAudioBuffer, &next_buffer_update, 1);
// }

// void removeAudioStream(uint8_t id) {
//   audio_stream* to_delete = audio_streams[id];
//   for (uint8_t i = id; i < num_audio_streams - 1; i ++) {
//     audio_streams[i] = audio_streams[i + 1];
//   }
//   if (to_delete->file.isOpen()) {
//     to_delete->file.close();
//   }
//   free(to_delete);
//   num_audio_streams -= 1;
// }

// void addTestStream(){
//   addAudioStream("recollections.pcm", 1);
// }

// void updateAudioBuffer(uint16_t allowed_time){
//   // Serial.println("updating audio buffer");
//   if (num_audio_streams == 0) {
//     return;
//   }
//   uint16_t empty = audio_buffer_loc - audio_buffer_end;
//   if (empty > 0 && empty <= FILL_SIZE) {
//     return;
//   }
//   uint16_t samples[FILL_SIZE];
//   for (uint8_t i = 0; i < num_audio_streams; i ++){
//     audio_stream* stream = audio_streams[i];
//     uint32_t fsize = stream->file.fileSize();
//     uint16_t amount_read = 0;
//     while (amount_read < FILL_SIZE * 2) {
//       uint16_t read_size = min(fsize - stream->file_loc, FILL_SIZE * 2);
//       stream->file.seek(stream->file_loc);
//       stream->file.read(samples, read_size);
//       if (i == 0) {
//         circular_memcpy(AUDIO_BUFFER, samples, audio_buffer_end * 2, AUDIO_BUFFER_SIZE * 2, read_size);
//       } else {
//         for (int j = 0; j < read_size / 2; j ++) {
//           AUDIO_BUFFER[(audio_buffer_end + j) % AUDIO_BUFFER_SIZE] += samples[j];
//         }
//       }
//       amount_read += read_size / 2;
//       stream->file_loc += read_size;
//       stream->file_loc %= fsize;
//       audio_buffer_end = (audio_buffer_end + read_size / 2) % AUDIO_BUFFER_SIZE;
//     }
//   }
//   uint16_t full = audio_buffer_end - audio_buffer_loc;
//   // Serial.print("Buffer now has ");
//   // Serial.println(full);
//   // Serial.println(AUDIO_BUFFER[101]);
//   uint16_t buffer_duration_millis = max(0, 1000 * (full - 200) / 44100);
//   next_buffer_update = millis() + buffer_duration_millis;
// }

// // https://forum.pjrc.com/index.php?threads/sdfat-read-write-example.65276/post-263415
// void initSd(){
//   Serial.print("Initializing SD card...");
//   // see if the card is present and can be initialized:
//   if (!sd.begin(SD_CONFIG)) {
//     Serial.println("initialization failed!");
//     return;
//   }
//   Serial.println("initialization done.");
// }

// // based on (/stolen from) https://gist.github.com/nonsintetic/ad13e70f164801325f5f552f84306d6f
// // ---------------------
// void audioTCConfigure()
// {
//  // select the generic clock generator used as source to the generic clock multiplexer
//  GCLK->CLKCTRL.reg = (uint16_t) (GCLK_CLKCTRL_CLKEN | GCLK_CLKCTRL_GEN_GCLK0 | GCLK_CLKCTRL_ID(GCM_TC4_TC5)) ;
//  while (GCLK->STATUS.bit.SYNCBUSY);

//  tcReset(); //reset TC5

//  // Set Timer counter 5 Mode to 8 bits, it will become a 8bit counter ('mode1' in the datasheet)
//  TC5->COUNT8.CTRLA.reg |= TC_CTRLA_MODE_COUNT8;
//  // Set TC5 waveform generation mode to 'normal frequency'
//  TC5->COUNT8.CTRLA.reg |= TC_CTRLA_WAVEGEN_NFRQ;
//  //set prescaler
//  //the clock normally counts at the GCLK_TC frequency, but we can set it to divide that frequency to slow it down
//  //you can use different prescaler divisons here like TC_CTRLA_PRESCALER_DIV1 to get a different range
//  TC5->COUNT8.CTRLA.reg |= TC_CTRLA_PRESCALER_DIV16 | TC_CTRLA_ENABLE; //it will divide GCLK_TC frequency by 16
//   // Set the PER register to 67 (resets every 68 ticks, 48m/16/68 is roughly 44k1)
 
//  TC5->COUNT8.PER.reg = (uint8_t) 67;
//  while (tcIsSyncing());
 
//  // Configure interrupt request
//  NVIC_DisableIRQ(TC5_IRQn);
//  NVIC_ClearPendingIRQ(TC5_IRQn);
//  NVIC_SetPriority(TC5_IRQn, 0);
//  NVIC_EnableIRQ(TC5_IRQn);

//  // Enable the TC5 interrupt request
//  TC5->COUNT8.INTENSET.bit.MC0 = 1;
//  while (tcIsSyncing()); //wait until TC5 is done syncing 
// }

// //Function that is used to check if TC5 is done syncing
// //returns true when it is done syncing
// bool tcIsSyncing()
// {
//   return TC5->COUNT8.STATUS.reg & TC_STATUS_SYNCBUSY;
// }

// //This function enables TC5 and waits for it to be ready
// void tcStartCounter()
// {
//   TC5->COUNT8.CTRLA.reg |= TC_CTRLA_ENABLE; //set the CTRLA register
//   while (tcIsSyncing()); //wait until snyc'd
// }

// //Reset TC5 
// void tcReset()
// {
//   TC5->COUNT8.CTRLA.reg = TC_CTRLA_SWRST;
//   while (tcIsSyncing());
//   while (TC5->COUNT8.CTRLA.bit.SWRST);
// }

// //disable TC5
// void tcDisable()
// {
//   TC5->COUNT8.CTRLA.reg &= ~TC_CTRLA_ENABLE;
//   while (tcIsSyncing());
// }
// // ---------------------

// void TC5_Handler(){
//   if (audio_buffer_loc != audio_buffer_end) {
//     analogWrite(DAC, AUDIO_BUFFER[audio_buffer_loc]);
//     audio_buffer_loc = (audio_buffer_loc + 1) % AUDIO_BUFFER_SIZE;
//     numSamples += 1;
//   }
//   TC5->COUNT8.INTFLAG.bit.MC0 = 1;
// }