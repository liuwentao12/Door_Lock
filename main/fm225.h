#ifndef FM225_H
#define FM225_H

void fm225_init(void);
void fm225_start_verify(void); // 发送命令，让它开始认人
void fm225_task(void *pvParameters); // 串口监听分身
void fm225_start_enroll(void);//注册人脸

#endif
