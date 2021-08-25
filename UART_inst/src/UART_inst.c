#include <stdbool.h> 
#include <stdio.h> 

#include "cmsis_os2.h" // CMSIS-RTOS 
 
// includes da biblioteca driverlib 
#include "inc/hw_memmap.h" 
#include "driverlib/gpio.h" 
#include "driverlib/uart.h" 
#include "driverlib/sysctl.h" 
#include "driverlib/pin_map.h" 
#include "utils/uartstdio.h" 
#include "system_TM4C1294.h" 
 
enum Estado{pega_mensagem = 0, manda_comando, aguarda_processo};


bool VerifySimulator(void);

osThreadId_t ElevadorEsquerda_thread_id, ElevadorCentral_thread_id, ElevadorDireita_thread_id, ControladorElevador_thread_id; 
osMutexId_t uart_id; 

osMessageQueueId_t filaControladora_id;
osMessageQueueId_t filaElevador_id[3];
osMessageQueueId_t filaElevador_backup_id[3];

 
const osThreadAttr_t thread1_attr = { 
  .name = "Thread 1" 
}; 
 
const osThreadAttr_t thread2_attr = { 
  .name = "Thread 2" 
}; 

const osThreadAttr_t thread3_attr = { 
  .name = "Thread 3" 
}; 
 
const osThreadAttr_t thread0_attr = { 
  .name = "Thread 0" 
}; 
 
//---------- 
// UART definitions 
extern void UARTStdioIntHandler(void); 
 
void UARTInit(void){ 
  // Enable UART0 
  SysCtlPeripheralEnable(SYSCTL_PERIPH_UART0); 
  while(!SysCtlPeripheralReady(SYSCTL_PERIPH_UART0)); 
 
  // Initialize the UART for console I/O. 
  UARTStdioConfig(0, 115200, SystemCoreClock); 
 
  // Enable the GPIO Peripheral used by the UART. 
  SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA); 
  while(!SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOA)); 
 
  // Configure GPIO Pins for UART mode. 
  GPIOPinConfigure(GPIO_PA0_U0RX); 
  GPIOPinConfigure(GPIO_PA1_U0TX); 
  GPIOPinTypeUART(GPIO_PORTA_BASE, GPIO_PIN_0 | GPIO_PIN_1); 
} // UARTInit 
 
void UART0_Handler(void){ 
  UARTStdioIntHandler(); 
} // UART0_Handler 
//---------- 
 
 
void myKernelInfo(void){ 
  osVersion_t osv; 
  char infobuf[16]; 
  if(osKernelGetInfo(&osv, infobuf, sizeof(infobuf)) == osOK) { 
    UARTprintf("Kernel Information: %s\n",   infobuf); 
    UARTprintf("Kernel Version    : %d\n",   osv.kernel); 
    UARTprintf("Kernel API Version: %d\n\n", osv.api); 
    UARTFlushTx(false); 
    UARTFlushRx(); 
  } // if 
} // myKernelInfo 
 
void myKernelState(void){ 
  UARTprintf("Kernel State: "); 
  switch(osKernelGetState()){ 
    case osKernelInactive: 
      UARTprintf("Inactive\n\n"); 
      break; 
    case osKernelReady: 
      UARTprintf("Ready\n\n"); 
      break; 
    case osKernelRunning: 
      UARTprintf("Running\n\n"); 
      break; 
    case osKernelLocked: 
      UARTprintf("Locked\n\n"); 
      break; 
    case osKernelError: 
      UARTprintf("Error\n\n"); 
      break; 
  } //switch 
  UARTFlushTx(false); 
} // myKernelState 
 
void myThreadState(osThreadId_t thread_id){ 
  osThreadState_t state; 
  state = osThreadGetState(thread_id); 
  switch(state){ 
  case osThreadInactive: 
    UARTprintf("Inactive\n"); 
    break; 
  case osThreadReady: 
    UARTprintf("Ready\n"); 
    break; 
  case osThreadRunning: 
    UARTprintf("Running\n"); 
    break; 
  case osThreadBlocked: 
    UARTprintf("Blocked\n"); 
    break; 
  case osThreadTerminated: 
    UARTprintf("Terminated\n"); 
    break; 
  case osThreadError: 
    UARTprintf("Error\n"); 
    break; 
  } // switch 
} // myThreadState 
 
void myThreadInfo(void){ 
  osThreadId_t threads[8]; 
  uint32_t number = osThreadEnumerate(threads, sizeof(threads)); 
   
  UARTprintf("Number of active threads: %d\n", number); 
  for(int n = 0; n < number; n++){ 
    UARTprintf("  %s (priority %d) - ", osThreadGetName(threads[n]), osThreadGetPriority(threads[n])); 
    myThreadState(threads[n]); 
  } // for 
  UARTprintf("\n"); 
  UARTFlushTx(false); 
} // myThreadInfo 
 
 
// osRtxIdleThread 
__NO_RETURN void osRtxIdleThread(void *argument){ 
  (void)argument; 
   
  while(1){ 
    //UARTprintf("Idle thread\n"); 
    asm("wfi"); 
  } // while 
} // osRtxIdleThread 
int andarAtual_e = 0;
int andarAtual_c = 0;
int andarAtual_d = 0;
int tickAnterior = 0;
__NO_RETURN void ControladorElevador(void *arg){ 
  uint32_t count = 0; 
  uint32_t tick; 
   
  int tamanhoBuffer = 10, podeEnviar =0, resultado; 
  char buffer[15], *resultado_s; 
  char buffer_c;
  char *_s; 
  
  while (1)
  {
    tick = osKernelGetTickCount();
    //if(tick - tickAnterior >= 172) {
      tick = osKernelGetTickCount(); 
      osMutexAcquire(uart_id, 2); 
        UARTgets(buffer , tamanhoBuffer); 
      
      UARTFlushRx();
      osMutexRelease(uart_id);
      
      osDelayUntil(tick + 1000);
      
      int andar = 0, interno = 0;
      
      if ((int)buffer[1] >= 49 && (int)buffer[1] <= 57) //fala qual andar está
      {
        andar = 1;
      }
      else
      {
        andar = 0;
      }
      
      if (strcmp(buffer[0], 'e') == 0)
      {
        if (andar)
        {
          andarAtual_e =((buffer[1]-0x30)*1);
        }
        else
        {
          osMessageQueuePut(filaElevador_backup_id[0], &buffer, 0, 0);
        }
        
      }
      else if (strcmp(buffer[0], 'c') == 0)
      {
        if (andar)
        {
          andarAtual_c =((buffer[1]-0x30)*1);
        }
        else
        {
          osMessageQueuePut(filaElevador_backup_id[1], &buffer, 0, 0);
          printf("botão\n");
        }
        
      }
      else if (strcmp(buffer[0], 'd') == 0)
      {
        if (andar)
        {
          andarAtual_d =((buffer[1]-0x30)*1);
        }
        else
        {
          osMessageQueuePut(filaElevador_backup_id[2], &buffer, 0, 0);
        }
      }
      
      /*
      if (strcmp(buffer, "cE01d") == 0)
      {
        
        int tick = osKernelGetTickCount(); 
        printf("subir");
        osMutexAcquire(uart_id, osWaitForever); 
        UARTprintf("cs\r", osThreadGetName(osThreadGetId()), 20); 
        UARTFlushTx(true);
        osMutexRelease(uart_id); 
        
        osDelayUntil(tick + 1000);
      
      
      }
       if (strcmp(buffer, "cE01s") == 0)
      {
        
        int tick = osKernelGetTickCount(); 
        printf("subir");
        osMutexAcquire(uart_id, osWaitForever); 
        UARTprintf("cf\r", osThreadGetName(osThreadGetId()), 20); 
        UARTFlushTx(true);
        osMutexRelease(uart_id); 
        
        osDelayUntil(tick + 1000);
      
      
      }
      if (strcmp(buffer, "dE01d") == 0)
      {
        
        int tick = osKernelGetTickCount(); 
        osMutexAcquire(uart_id, osWaitForever); 
        UARTprintf("cx\r", osThreadGetName(osThreadGetId()), 20); 
        UARTFlushTx(true);
        osMutexRelease(uart_id); 
        
        osDelayUntil(tick + 1000);
      
      
      }
      if (strcmp(buffer, "eE01d") == 0)
      {
        
        int tick = osKernelGetTickCount(); 
        osMutexAcquire(uart_id, osWaitForever); 
        UARTprintf("cj\r", osThreadGetName(osThreadGetId()), 20); 
        UARTFlushTx(true);
        osMutexRelease(uart_id); 
        
        osDelayUntil(tick + 1000);
      
      
      }
      
      if (strcmp(buffer, "dE01s") == 0)
      {
        
        int tick = osKernelGetTickCount(); 
        osMutexAcquire(uart_id, osWaitForever); 
        UARTprintf("cp\r", osThreadGetName(osThreadGetId()), 20); 
        UARTFlushTx(true);
        osMutexRelease(uart_id); 
        
        osDelayUntil(tick + 1000);
      
      
      }
      
      if (strcmp(buffer, "eE01s") == 0)
      {
        
        int tick = osKernelGetTickCount(); 
        osMutexAcquire(uart_id, osWaitForever); 
        UARTprintf("cA\r", osThreadGetName(osThreadGetId()), 20); 
        UARTFlushTx(true);
        osMutexRelease(uart_id); 
        
        osDelayUntil(tick + 1000);
      
      
      }
      */
      tickAnterior = tick;
  }
  
}


__NO_RETURN void ElevadorDireita(void *arg){ 
  uint32_t count = 0; 
  uint32_t tick; 
   
  int tamanhoBuffer = 10, podeEnviar =0, resultado; 
  char buffer[15], *resultado_s; 
  char *_s; 
  
  //filaElevador_backup_id - backup
  int ocupado= 0;
  int andarAtual = 0, qtdMensagens = 0, qtdMensagens_andar = 0, qtdMensagensAtualizado = 0;
  
  enum Estado estado = pega_mensagem;
  char mensagemAtual[3];
  int proximoAndar;
  
  while (true){
    switch (estado)
    {
      case pega_mensagem:
        if (osMessageQueueGetCount(filaElevador_backup_id[2]) != 0)
        {
          osMessageQueueGet (filaElevador_backup_id[2], &buffer, NULL, osWaitForever);
          estado = manda_comando;
        
        }
        
        if (buffer[0] == 'ÿ')
        {
          memmove(buffer, buffer+1, strlen(buffer));
        }
        
        break;
      case manda_comando:
        if (buffer[1] == 'I')
        {
          proximoAndar =(int) buffer[2] - 97;
        }
        else if (buffer[1]=='E')
        {
          proximoAndar =((buffer[2]-0x30)*10)+((buffer[3]-0x30));
          
        }
        
        osMutexAcquire(uart_id, 2); 
        UARTprintf("df\r", osThreadGetName(osThreadGetId()), 20); 
        UARTFlushTx(true);
        osMutexRelease(uart_id); 
        
        if (proximoAndar > andarAtual_d)
          {
            osMutexAcquire(uart_id, 2); 
            UARTprintf("ds\r", osThreadGetName(osThreadGetId()), 20); 
            UARTFlushTx(true);
            osMutexRelease(uart_id); 
          }
          else if (proximoAndar < andarAtual_d)
          {
            osMutexAcquire(uart_id, 2); 
            UARTprintf("dd\r", osThreadGetName(osThreadGetId()), 20); 
            UARTFlushTx(true);
            osMutexRelease(uart_id); 
          }
          
          estado = aguarda_processo;
        
        break;
      case aguarda_processo:
          int tickAntigo = 0, tick = 0;
          do
          {
            tick = osKernelGetTickCount(); 
            
             if (tick - tickAnterior > 1000)
             {
                if (proximoAndar > andarAtual_d)
                {
                  osMutexAcquire(uart_id, 2); 
                  UARTprintf("ds\r", osThreadGetName(osThreadGetId()), 20); 
                  UARTFlushTx(true);
                  osMutexRelease(uart_id); 
                }
                else if (proximoAndar < andarAtual_d)
                {
                  osMutexAcquire(uart_id, 2); 
                  UARTprintf("dd\r", osThreadGetName(osThreadGetId()), 20); 
                  UARTFlushTx(true);
                  osMutexRelease(uart_id); 
                }
             }
            
            if (proximoAndar == andarAtual_d)
            {
              osMutexAcquire(uart_id, 2); 
              UARTprintf("dp\r", osThreadGetName(osThreadGetId()), 20); 
              UARTprintf("da\r", osThreadGetName(osThreadGetId()), 20); 
              UARTFlushTx(true);
              osMutexRelease(uart_id); 
              estado = pega_mensagem;
            }
            tickAnterior = tick;
          } while (estado == aguarda_processo);
          
        
        
        break;
    }
    
  }
} // thread2 


__NO_RETURN void ElevadorEsquerda(void *arg){ 
  uint32_t count = 0; 
  uint32_t tick; 
   
  int tamanhoBuffer = 10, podeEnviar =0, resultado; 
  char buffer[15], *resultado_s; 
  char *_s; 
  
  //filaElevador_backup_id - backup
  int ocupado= 0;
  int andarAtual = 0, qtdMensagens = 0, qtdMensagens_andar = 0, qtdMensagensAtualizado = 0;
  
  enum Estado estado = pega_mensagem;
  char mensagemAtual[3];
  int proximoAndar;
  
  while (true){
    switch (estado)
    {
      case pega_mensagem:
        if (osMessageQueueGetCount(filaElevador_backup_id[0]) != 0)
        {
          osMessageQueueGet (filaElevador_backup_id[0], &buffer, NULL, osWaitForever);
          estado = manda_comando;
        
        }
        
        if (buffer[0] == 'ÿ')
        {
          memmove(buffer, buffer+1, strlen(buffer));
        }
        
        break;
      case manda_comando:
        if (buffer[1] == 'I')
        {
          proximoAndar =(int) buffer[2] - 97;
        }
        else if (buffer[1]=='E')
        {
          proximoAndar =((buffer[2]-0x30)*10)+((buffer[3]-0x30));
          
        }
        
        osMutexAcquire(uart_id, 2); 
        UARTprintf("ef\r", osThreadGetName(osThreadGetId()), 20); 
        UARTFlushTx(true);
        osMutexRelease(uart_id); 
        
        if (proximoAndar > andarAtual_e)
          {
            osMutexAcquire(uart_id, 2); 
            UARTprintf("es\r", osThreadGetName(osThreadGetId()), 20); 
            UARTFlushTx(true);
            osMutexRelease(uart_id); 
          }
          else if (proximoAndar < andarAtual_e)
          {
            osMutexAcquire(uart_id, 2); 
            UARTprintf("ed\r", osThreadGetName(osThreadGetId()), 20); 
            UARTFlushTx(true);
            osMutexRelease(uart_id); 
          }
          
          estado = aguarda_processo;
        
        break;
      case aguarda_processo:
          int tickAntigo = 0, tick = 0;
          do
          {
            tick = osKernelGetTickCount(); 
            
             if (tick - tickAnterior > 1000)
             {
                if (proximoAndar > andarAtual_e)
                {
                  osMutexAcquire(uart_id, 2); 
                  UARTprintf("es\r", osThreadGetName(osThreadGetId()), 20); 
                  UARTFlushTx(true);
                  osMutexRelease(uart_id); 
                }
                else if (proximoAndar < andarAtual_e)
                {
                  osMutexAcquire(uart_id, 2); 
                  UARTprintf("ed\r", osThreadGetName(osThreadGetId()), 20); 
                  UARTFlushTx(true);
                  osMutexRelease(uart_id); 
                }
             }
            
            if (proximoAndar == andarAtual_e)
            {
              osMutexAcquire(uart_id, 2); 
              UARTprintf("ep\r", osThreadGetName(osThreadGetId()), 20); 
              UARTprintf("ea\r", osThreadGetName(osThreadGetId()), 20); 
              UARTFlushTx(true);
              osMutexRelease(uart_id); 
              estado = pega_mensagem;
            }
            tickAnterior = tick;
          } while (estado == aguarda_processo);
          
        
        
        break;
    }
    
  }
} 
 


__NO_RETURN void ElevadorCentral(void *arg){ 
  uint32_t count = 0; 
  uint32_t tick; 
   
  int tamanhoBuffer = 10, podeEnviar =0, resultado; 
  char buffer[15], *resultado_s; 
  char *_s; 
  
  //filaElevador_backup_id - backup
  int ocupado= 0;
  int andarAtual = 0, qtdMensagens = 0, qtdMensagens_andar = 0, qtdMensagensAtualizado = 0;
  
  enum Estado estado = pega_mensagem;
  char mensagemAtual[3];
  int proximoAndar;
  
  while (true){
    switch (estado)
    {
      case pega_mensagem:
        if (osMessageQueueGetCount(filaElevador_backup_id[1]) != 0)
        {
          osMessageQueueGet (filaElevador_backup_id[1], &buffer, NULL, osWaitForever);
          estado = manda_comando;
        
        }
        
        if (buffer[0] == 'ÿ')
        {
          memmove(buffer, buffer+1, strlen(buffer));
        }
        
        break;
      case manda_comando:
        if (buffer[1] == 'I')
        {
          proximoAndar =(int) buffer[2] - 97;
        }
        else if (buffer[1]=='E')
        {
          proximoAndar =((buffer[2]-0x30)*10)+((buffer[3]-0x30));
          
        }
        
        osMutexAcquire(uart_id, 2); 
        UARTprintf("cf\r", osThreadGetName(osThreadGetId()), 20); 
        UARTFlushTx(true);
        osMutexRelease(uart_id); 
        
        if (proximoAndar > andarAtual_c)
          {
            osMutexAcquire(uart_id, 2); 
            UARTprintf("cs\r", osThreadGetName(osThreadGetId()), 20); 
            UARTFlushTx(true);
            osMutexRelease(uart_id); 
          }
          else if (proximoAndar < andarAtual_c)
          {
            osMutexAcquire(uart_id, 2); 
            UARTprintf("cd\r", osThreadGetName(osThreadGetId()), 20); 
            UARTFlushTx(true);
            osMutexRelease(uart_id); 
          }
          
          estado = aguarda_processo;
        
        break;
      case aguarda_processo:
          int tickAntigo = 0, tick = 0;
          do
          {
            tick = osKernelGetTickCount(); 
            
             if (tick - tickAnterior > 1000)
             {
                if (proximoAndar > andarAtual_c)
                {
                  osMutexAcquire(uart_id, 2); 
                  UARTprintf("cs\r", osThreadGetName(osThreadGetId()), 20); 
                  UARTFlushTx(true);
                  osMutexRelease(uart_id); 
                }
                else if (proximoAndar < andarAtual_c)
                {
                  osMutexAcquire(uart_id, 2); 
                  UARTprintf("cd\r", osThreadGetName(osThreadGetId()), 20); 
                  UARTFlushTx(true);
                  osMutexRelease(uart_id); 
                }
             }
            
            if (proximoAndar == andarAtual_c)
            {
              osMutexAcquire(uart_id, 2); 
              UARTprintf("cp\r", osThreadGetName(osThreadGetId()), 20); 
              
              UARTprintf("ca\r", osThreadGetName(osThreadGetId()), 20); 
              UARTFlushTx(true);
              osMutexRelease(uart_id); 
              estado = pega_mensagem;
            }
            tickAnterior = tick;
          } while (estado == aguarda_processo);
          
        
        
        break;
    }
    
  }
} 
 
void main(void){ 
  UARTInit(); 
  
  if(osKernelGetState() == osKernelInactive) 
     osKernelInitialize(); 
 
  int qtdMensagem = 15;
  filaElevador_id[0] = osMessageQueueNew(qtdMensagem, sizeof(char*), NULL);
  filaElevador_id[1] = osMessageQueueNew(qtdMensagem, sizeof(char*), NULL);
  filaElevador_id[2] = osMessageQueueNew(qtdMensagem, sizeof(char*), NULL);
  filaElevador_backup_id[0] = osMessageQueueNew(qtdMensagem, sizeof(char*), NULL);
  filaElevador_backup_id[1] = osMessageQueueNew(qtdMensagem, sizeof(char*), NULL);
  filaElevador_backup_id[2] = osMessageQueueNew(qtdMensagem, sizeof(char*), NULL);
  filaControladora_id = osMessageQueueNew(qtdMensagem, sizeof(char*), NULL);
  
  
  if (filaElevador_id[0] != NULL) 
    printf("criado");
  
  char *buffer = "c0";
  osMessageQueuePut(filaElevador_id[1], &buffer, 0, 0);
  
  buffer = "e0";
  osMessageQueuePut(filaElevador_id[0], &buffer, 0, 0);
  
  buffer = "d0";
  osMessageQueuePut(filaElevador_id[2], &buffer, 0, 0);
  
  ControladorElevador_thread_id = osThreadNew(ControladorElevador, NULL, &thread0_attr); 
  
  
  ElevadorCentral_thread_id = osThreadNew(ElevadorCentral, NULL, &thread2_attr); 
  ElevadorEsquerda_thread_id = osThreadNew(ElevadorEsquerda, NULL, &thread1_attr); 
  ElevadorDireita_thread_id = osThreadNew(ElevadorDireita, NULL, &thread3_attr); 

  uart_id = osMutexNew(NULL); 
  
  
  osMutexAcquire(uart_id, osWaitForever); 
  UARTprintf("er\r", osThreadGetName(osThreadGetId()), 10);
  UARTprintf("cr\r", osThreadGetName(osThreadGetId()), 10);
  UARTprintf("dr\r", osThreadGetName(osThreadGetId()), 10);
  UARTprintf("er\r", osThreadGetName(osThreadGetId()), 10);
  UARTprintf("cr\r", osThreadGetName(osThreadGetId()), 10);
  UARTprintf("dr\r", osThreadGetName(osThreadGetId()), 10);
  UARTprintf("ef\r", osThreadGetName(osThreadGetId()), 10);
  UARTprintf("cf\r", osThreadGetName(osThreadGetId()), 10);
  UARTprintf("df\r", osThreadGetName(osThreadGetId()), 10);
  UARTFlushTx(true);
  osMutexRelease(uart_id); 
 
  if(osKernelGetState() == osKernelReady) 
    osKernelStart(); 
  
  
 
  while(1); 
} // main


bool VerifySimulator(void){
  bool verified = false;
  int tamanhoBuffer = 15, tick;
  char buffer[15];
  
  tick = osKernelGetTickCount(); 
  osMutexAcquire(uart_id, osWaitForever); 
  UARTgets(buffer , tamanhoBuffer); 
  UARTFlushRx();
  osMutexRelease(uart_id); 
  
  osDelayUntil(tick + 1000); // bloqueio simulando T 
  
  if (strcmp(buffer, "initialized") == 0)
  {
    return true;
  }
  else
  {
    return false;
  }
}