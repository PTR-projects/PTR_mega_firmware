const uint8_t aclOdr[12] = {0, 11, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10}
const uint8_t gyroOdr[11] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10}

const uint8_t aclScale[4] = {0, 2, 3, 1};
const uint8_t gyroScale[5] = {4, 0, 1, 2 ,3}

void SPI_write(uint8_t* dataArr, uint8_t len, uint8_t EN_PIN) {
    //Super kod od SPI
}

void SPI_read(uint8_t* dataArr, uint8_t len, uint8_t EN_PIN) {
    //Super kod od SPI
    
}

bool LSM_begin(uint8_t EN_PIN) {

    SPI_write(&settings3c, FIFO_CTRL, 1, EN_PIN);
    SPI_write(&settingsFIFO, FIFO_CTRL_5, 1, EN_PIN);
    

}

void LSM_gyroSettings(uint8_t settings, uint8_t resolution, uint8_t ODR, uint8_t LPF, uint8_t EN_PIN) {
    
    defaultSettingsGyro = (ODR << 4) | (resolution << 1);

    SPI_write(&LPF, CTRL6_C, 1, EN_PIN);
    SPI_write(&settings, CTRL7_G, 1, EN_PIN);
    SPI_write(&defaultSettingsGyro, CTRL2_G, 1, EN_PIN);
}

void LSM_aclSettings(uint8_t settings, uint8_t resolution, uint8_t ODR, uint8_t LPF, uint8_t EN_PIN) {

    defaultSettingsAcl = (ODR << 4) | (resolution << 2);

    SPI_write(&LPF, CTRL8_XL, 1, EN_PIN);
    SPI_write(&defaultSettingsAcl, CTRL1_XL, 1, EN_PIN);

}

void LSM_setDataRate(uint8_t aclRate, uint8_t gyroRate, uint8_t EN_PIN);

void LSM_getRawData(uint8_t* rawData, uint8_t EN_PIN) {

    const uint8_t packet = OUTX_L_G;

    SPI_write(&packet, 1, EN_PIN);
    SPI_read(rawData, 12, EN_PIN);
}

void LSM_getData(uint8_t* sensorData, uint8_t EN_PIN) {
   


}

void softReset(uint8_t EN_PIN) {
    settings |= 0b10000000;
    SPI_write(&settings3c, CTRL_3, 1, EN_PIN);
    settings &= 0b01111111;
}