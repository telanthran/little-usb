#ifndef ACCELEROMETER_H_
#define ACCELEROMETER_H_

typedef struct {
    void (*init)(void);
    float (*read_x_axis)(void);
    float (*read_y_axis)(void);
} AccelerometerDriver;

#endif /* ACCELEROMETER_H_ */