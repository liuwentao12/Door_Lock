#ifndef LCD1602_H
#define LCD1602_H

// 初始化 LCD1602 屏幕
void lcd1602_init(void);

// 在指定行显示字符串
// 参数 line: 0 代表第一行，1 代表第二行
// 参数 str: 要显示的英文字符串 (不能超 16 个字符)
void lcd1602_print(int line, const char* str);

// 清空屏幕
void lcd1602_clear(void);

#endif
