#ifndef CONFIG_H_
#define CONFIG_H_

#define DEBUG_1(x) Serial.println((x));
#define DEBUG_2(n, x)  \
    Serial.print((n)); \
    Serial.println((x));

#endif