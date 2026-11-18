idf_component_register(SRCS "TMTC.c"
                    INCLUDE_DIRS "include"
                    REQUIRES LORA_driver PTR_DataPacket
                    PRIV_REQUIRES Cansat_driver Preferences CONFIG)
