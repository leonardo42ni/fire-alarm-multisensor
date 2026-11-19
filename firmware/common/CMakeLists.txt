if(DEFINED ESP_PLATFORM)
    idf_component_register(
        SRCS "proto_ndjson.c" "proto_ndjson_json.c"
             "proto_ndjson_dec.c" "proto_ndjson_enc.c"
        INCLUDE_DIRS ".")
else()
    add_library(fire_common STATIC
        "proto_ndjson.c" "proto_ndjson_json.c"
        "proto_ndjson_dec.c" "proto_ndjson_enc.c")
    target_include_directories(fire_common PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
endif()
