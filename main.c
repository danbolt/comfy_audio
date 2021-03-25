/*
   main.c

   NuSYSTEM sample nu0

   Copyright (C) 1997-1999, NINTENDO Co,Ltd.	
   */

#include <nusys.h>

#include <libaudio.h>

#define COMFY_SAMPLE_OUTPUT_RATE 32000

#define COMFY_MAX_VOICES 24
#define COMFY_MAX_UPDATES 32

#define COMFY_HEAP_SIZE   0x50000 /* Default heap size as NuSys does it */
#define COMFY_HEAP_ADDR   (NU_GFX_FRAMEBUFFER_ADDR - COMFY_HEAP_SIZE)

#define COMFY_AUDIO_THREAD_PRIORITY 12

#define COMFY_NUM_INTERRUPTS 1 /* 60Hz */

#define COMFY_EXTRA_SAMPLES 80

#define COMFY_NUMBER_OF_DMA_BUFFERS 24 

ALHeap comfy_AudioHeap;
ALGlobals comfy_AudioGlobals;

/* These are copied as from NuSys and other game examples */
typedef struct {
    ALLink  node; 
    s32   startAddr; 
    s32   frameCnt;
    char* ptr;
} comfy_DMABuffer;

typedef struct {
    u8    initialized;
    comfy_DMABuffer* firstUsed;
    comfy_DMABuffer* firstFree;
} comfy_DMAState;

static comfy_DMAState dmaState;
static comfy_DMABuffer dmaBuffers[COMFY_NUMBER_OF_DMA_BUFFERS];

static u32 frameSize;
static u32 minFrameSize;
static u32 maxFrameSize;

static ALDMAproc comfy_DMANew(comfy_DMAState **state) {
  //
}

void comfy_setupAudioThread(ALSynConfig* config) {
  int i;
  float fractionalFrameSize;

  dmaState.initialized = FALSE;

  fractionalFrameSize = ((float)(COMFY_NUM_INTERRUPTS)) * config->outputRate / 60.f;
  frameSize = (s32)fractionalFrameSize;
  if (frameSize < fractionalFrameSize) {
    frameSize++;
  }
  if (frameSize & 0xf) {
    frameSize = (frameSize & ~0xf) + 0x10;
  }
  minFrameSize = frameSize - 16;
  maxFrameSize = frameSize + COMFY_EXTRA_SAMPLES + 16;

  alInit(&comfy_AudioGlobals, config);

  dmaBuffers[0].node.next = NULL;
  dmaBuffers[0].node.prev = NULL;
  for (i = 0; i < COMFY_NUMBER_OF_DMA_BUFFERS - 1; i++) {
    alLink((ALLink*)&dmaBuffers[i+1], (ALLink*)&dmaBuffers[i]);
  }

}

void comfy_initializeAudioHeap(void* heapAddressStart, u32 heapSize) {
  alHeapInit(&comfy_AudioHeap, heapAddressStart, heapSize);
}

s32 comfy_getAudioHeapAmountUsed(void) {
    return ((s32)comfy_AudioHeap.cur - (s32)comfy_AudioHeap.base);
}

s32 comfy_getAudioHeapAmountFree(void) {
    return (comfy_AudioHeap.len - comfy_getAudioHeapAmountUsed());
}

s32 comfy_initialize(void) {
  ALSynConfig audioConfig;
  audioConfig.maxVVoices = COMFY_MAX_VOICES;
  audioConfig.maxPVoices = COMFY_MAX_VOICES;
  audioConfig.maxUpdates = COMFY_MAX_UPDATES;
  audioConfig.fxType = AL_FX_SMALLROOM; // TODO: why not let the user set this on init?
  audioConfig.dmaproc = comfy_DMANew;
  audioConfig.outputRate = osAiSetFrequency(COMFY_SAMPLE_OUTPUT_RATE);

  comfy_initializeAudioHeap((void*)COMFY_HEAP_ADDR, COMFY_HEAP_SIZE);
  audioConfig.heap = &comfy_AudioHeap;

  comfy_setupAudioThread(&audioConfig);

  return comfy_getAudioHeapAmountUsed();
}

/* Declaration of the prototype */
void stage00(int);
void makeDL00(void);

/*------------------------
	Main
--------------------------*/
void mainproc(void)
{
  /* The initialization of graphic  */
  nuGfxInit();

  comfy_initialize();

  /* Register call-back  */
  nuGfxFuncSet((NUGfxFunc)stage00);
  /* The screen display ON */
  nuGfxDisplayOn();

  while(1)
    ;
}

/*-----------------------------------------------------------------------------
  The call-back function 

  pendingGfx which is passed from Nusystem as the argument of the call-back 
  function is the total number of RCP tasks that are currently processing 
  and waiting for the process. 
-----------------------------------------------------------------------------*/
void stage00(int pendingGfx)
{
  /* It provides the display process if there is no RCP task that is processing. */
  if(pendingGfx < 1)
    makeDL00();		
}

