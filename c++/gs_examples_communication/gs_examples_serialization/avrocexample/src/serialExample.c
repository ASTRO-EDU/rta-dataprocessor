#include <stdio.h>
#include <stdlib.h>
#include <avro.h>

#include "hk-schema.h"
// Define the JSON schema as a char[] array

avro_schema_t hk_schema;
int64_t id = 0;

/* Parse schema into a schema data structure */
void init_schema(void)
{
        if (avro_schema_from_json_literal(HK, &hk_schema)) {
                fprintf(stderr, "Unable to parse hk schema\n");
                exit(EXIT_FAILURE);
        }
}

/* Create a value to match the person schema and save it */
void
add_hk(avro_file_writer_t db, const char *apid, const char *type, const int *runID,
           const char *phone, int32_t age)
{
        avro_value_iface_t  *person_class =
            avro_generic_class_from_schema(hk_schema);

        avro_value_t  hk_p;
        avro_generic_value_new(person_class, &hk_p);

        avro_value_t apid_value;
        avro_value_t counter;
        avro_value_t last_value;
        avro_value_t epoch_time_value;
        avro_value_t time_value;
        avro_value_t run_value;
        avro_value_t crc_value;
        avro_value_t flags_value;
        avro_value_t cfg_value;
        avro_value_t wfcount_value;

        if (avro_value_get_by_name(&hk_p, "apid", &apid_value, NULL) == 0) {
                avro_value_set_long(&apid_value, apid);
        }
        if (avro_value_get_by_name(&hk_p, "counter", &counter, NULL) == 0) {
                avro_value_set_string(&counter, id++);
        }
        if (avro_value_get_by_name(&hk_p, "type", &last_value, NULL) == 0) {
                avro_value_set_string(&last_value, type);
        }
        if (avro_value_get_by_name(&hk_p, "abstime", &epoch_time_value, NULL) == 0) {
                avro_value_set_int(&epoch_time_value, (unsigned)time(NULL));
        }
        if (avro_value_get_by_name(&hk_p, "runID", &run_value, NULL) == 0) {
                avro_value_set_int(&run_value, runID);
        }
        if (avro_value_get_by_name(&hk_p, "configID", &cfg_value, NULL) == 0) {
                avro_value_set_int(&cfg_value, 0);
        }
        if (avro_value_get_by_name(&hk_p, "wformcount", &wfcount_value, NULL) == 0) {
                avro_value_set_int(&wfcount_value, 1);
        }
        if (avro_value_get_by_name(&hk_p, "flags", &flags_value, NULL) == 0) {
                avro_value_set_int(&flags_value, 2);
        }
        if (avro_value_get_by_name(&hk_p, "crc", &crc_value, NULL) == 0) {
                avro_value_set_int(&crc_value, 3);
        }
        if (avro_value_get_by_name(&hk_p, "time", &run_value, NULL) == 0) {
            if (avro_value_get_by_name(&hk_p, "crc", &run_value, NULL) == 0) {
            
            }
            if (avro_value_get_by_name(&hk_p, "crc", &run_value, NULL) == 0) {
                
            }
        }

        if (avro_file_writer_append_value(db, &hk_p)) {
                fprintf(stderr,
                        "Unable to write Person value to memory buffer\nMessage: %s\n", avro_strerror());
                exit(EXIT_FAILURE);
        }

        /* Decrement all our references to prevent memory from leaking */
        avro_value_decref(&hk_p);
        avro_value_iface_decref(person_class);
}


int main() {
    avro_schema_t schema;
    avro_value_iface_t *iface;
    avro_value_t value;
    avro_value_t field;
    avro_writer_t writer;
    FILE *file;

    // Create schema from JSON literal
    schema = avro_schema_from_json_literal(HK, NULL);
    if (schema == NULL) {
        fprintf(stderr, "Failed to create schema from JSON.\n");
        return 1;
    }

    // Create a new writer to write the data
    file = fopen("data.avro", "wb");
    if (file == NULL) {
        perror("Failed to open file");
        return 1;
    }

    writer = avro_writer_file(file);
    if (writer == NULL) {
        fprintf(stderr, "Failed to create Avro writer.\n");
        fclose(file);
        return 1;
    }

    // Create a value interface and instance based on the schema
    iface = avro_generic_class_from_schema(schema);
    if (iface == NULL) {
        fprintf(stderr, "Failed to create value interface from schema.\n");
        avro_writer_free(writer);
        fclose(file);
        return 1;
    }

    avro_value_iface(iface, "HeaderHK");
    avro_value_new(iface, &value);

    // Set the values for each field
    avro_value_get_by_name(&value, "apid", &field, NULL);
    avro_value_set_int(&field, 123);

    avro_value_get_by_name(&value, "counter", &field, NULL);
    avro_value_set_int(&field, 456);

    avro_value_get_by_name(&value, "type", &field, NULL);
    avro_value_set_int(&field, 789);

    avro_value_get_by_name(&value, "abstime", &field, NULL);
    avro_value_set_int(&field, 101);

    avro_value_get_by_name(&value, "runID", &field, NULL);
    avro_value_set_int(&field, 202);

    avro_value_get_by_name(&value, "configID", &field, NULL);
    avro_value_set_int(&field, 303);

    avro_value_get_by_name(&value, "wformcount", &field, NULL);
    avro_value_set_int(&field, 404);

    avro_value_get_by_name(&value, "flags", &field, NULL);
    avro_value_set_int(&field, 505);

    avro_value_get_by_name(&value, "crc", &field, NULL);
    avro_value_set_int(&field, 606);

    // Write the data to the Avro file
    avro_writer_write(writer, &value);

    // Clean up
    avro_value_decref(&value);
    avro_writer_free(writer);
    fclose(file);

    return 0;
}
