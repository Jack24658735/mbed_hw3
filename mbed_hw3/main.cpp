#include "mbed.h"
#include "fsl_port.h"
#include "fsl_gpio.h"
#define _USE_MATH_DEFINES
#include <math.h>

#define UINT14_MAX        16383
// FXOS8700CQ I2C address
#define FXOS8700CQ_SLAVE_ADDR0 (0x1E<<1) // with pins SA0=0, SA1=0
#define FXOS8700CQ_SLAVE_ADDR1 (0x1D<<1) // with pins SA0=1, SA1=0
#define FXOS8700CQ_SLAVE_ADDR2 (0x1C<<1) // with pins SA0=0, SA1=1
#define FXOS8700CQ_SLAVE_ADDR3 (0x1F<<1) // with pins SA0=1, SA1=1
// FXOS8700CQ internal register addresses
#define FXOS8700Q_STATUS 0x00
#define FXOS8700Q_OUT_X_MSB 0x01
#define FXOS8700Q_OUT_Y_MSB 0x03
#define FXOS8700Q_OUT_Z_MSB 0x05
#define FXOS8700Q_M_OUT_X_MSB 0x33
#define FXOS8700Q_M_OUT_Y_MSB 0x35
#define FXOS8700Q_M_OUT_Z_MSB 0x37
#define FXOS8700Q_WHOAMI 0x0D
#define FXOS8700Q_XYZ_DATA_CFG 0x0E
#define FXOS8700Q_CTRL_REG1 0x2A
#define FXOS8700Q_M_CTRL_REG1 0x5B
#define FXOS8700Q_M_CTRL_REG2 0x5C
#define FXOS8700Q_WHOAMI_VAL 0xC7

DigitalOut led2(LED2);
I2C i2c(PTD9,PTD8);
Serial pc(USBTX, USBRX);
InterruptIn sw2(SW2);
int m_addr = FXOS8700CQ_SLAVE_ADDR1;

int i = 0; // global index for saving logged data

float x_value[100];
float y_value[100];
float z_value[100];
int logged_value[100];
EventQueue main_q(100 * EVENTS_EVENT_SIZE);
EventQueue q1(100 * EVENTS_EVENT_SIZE);
EventQueue q2(100 * EVENTS_EVENT_SIZE);

Thread main_t;
Thread t1;
Thread t2;

void FXOS8700CQ_readRegs(int addr, uint8_t * data, int len);
void FXOS8700CQ_writeRegs(uint8_t * data, int len);

void send_data(float *data);

void tilt_logger(int x);
void logger_and_LED();
void led_blink(int x);

int main() {
    main_t.start(callback(&main_q, &EventQueue::dispatch_forever));
    pc.baud(115200);

    sw2.rise(main_q.event(logger_and_LED));
    while (1) {
        wait(1);
    }
}

void logger_and_LED() {
    wait(0.5); // wait a second for the data to be stable
    t1.start(callback(&q1, &EventQueue::dispatch_forever));
    t2.start(callback(&q2, &EventQueue::dispatch_forever));

    q1.call_every(100, tilt_logger, 0);
    q2.call_every(1000, led_blink, 0);

    wait(10); // wait for 10 seconds
    t1.terminate(); // stop the thread
    t2.terminate();

    // send data
    send_data(x_value);
    send_data(y_value);
    send_data(z_value);

    // send logger data
    for (int i = 0; i < 100; ++i) {
        wait(0.05);
        pc.printf("%d\r\n", logged_value[i]);
    }
    
}

void FXOS8700CQ_readRegs(int addr, uint8_t * data, int len) {
   char t = addr;
   i2c.write(m_addr, &t, 1, true);
   i2c.read(m_addr, (char *)data, len);
}

void FXOS8700CQ_writeRegs(uint8_t * data, int len) {
   i2c.write(m_addr, (char *)data, len);
}

void send_data(float *data) {
    for (int i = 0; i < 100; ++i) {
        wait(0.05);
        pc.printf("%1.3f\r\n", data[i]);
    }
}

void led_blink(int x) {
    led2 = !led2;
}

void tilt_logger(int x) {
    uint8_t who_am_i, data[2], res[6];
    int16_t acc16;
    float t[3];

    // Enable the FXOS8700Q

    FXOS8700CQ_readRegs( FXOS8700Q_CTRL_REG1, &data[1], 1);
    data[1] |= 0x01;
    data[0] = FXOS8700Q_CTRL_REG1;
    FXOS8700CQ_writeRegs(data, 2);

    // Get the slave address
    // FXOS8700CQ_readRegs(FXOS8700Q_WHOAMI, &who_am_i, 1);

    //    pc.printf("Here is %x\r\n", who_am_i);
    //while (true) {
        
        FXOS8700CQ_readRegs(FXOS8700Q_OUT_X_MSB, res, 6);

        acc16 = (res[0] << 6) | (res[1] >> 2);
        if (acc16 > UINT14_MAX/2)
            acc16 -= UINT14_MAX;
        t[0] = ((float)acc16) / 4096.0f;

        acc16 = (res[2] << 6) | (res[3] >> 2);
        if (acc16 > UINT14_MAX/2)
            acc16 -= UINT14_MAX;
        t[1] = ((float)acc16) / 4096.0f;

        acc16 = (res[4] << 6) | (res[5] >> 2);
        if (acc16 > UINT14_MAX/2)
            acc16 -= UINT14_MAX;
        t[2] = ((float)acc16) / 4096.0f;

        x_value[i] = t[0];
        y_value[i] = t[1];
        z_value[i] = t[2];

        // calculate the result
        float result = acos(z_value[i] / sqrt(x_value[i] * x_value[i] + y_value[i] * y_value[i] + z_value[i] * z_value[i]));
        if (result > (M_PI / 4))
            logged_value[i] = 1;
        else
            logged_value[i] = 0;

        i++;
        
        /*
        printf("FXOS8700Q ACC: X=%1.4f(%x%x) Y=%1.4f(%x%x) Z=%1.4f(%x%x)\r\n",\
                t[0], res[0], res[1],\
                t[1], res[2], res[3],\
                t[2], res[4], res[5]\
        );*/

        // wait(1.0);
   // }
}



