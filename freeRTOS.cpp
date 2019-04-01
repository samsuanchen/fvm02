#include <fvm.h>
extern FVM F;
void Task0(void *pvParameters) { 
  while (true) { F.update(); vTaskDelay(10); (void) pvParameters; }
}
void xxx(int cpu){
  xTaskCreatePinnedToCore(
    Task0,            // pvTaskCode,Pointer to the task entry function */
    "Task0",          // pcName, A descriptive name for the task. */
    1024,             // usStackDepth, The size of the task stack specified as the number of bytes */
    NULL,             // pvParameters, Pointer that will be used as the parameter for the task being created */
    1,                // uxPriority, The priority at which the task should run */
    NULL,             // pxCreatedTask, Used to pass back a handle by which the created task can be referenced */
//	cpu
	tskNO_AFFINITY    // xCoreID,  Specify the number of the core which the task should be pinned to */
  );
}