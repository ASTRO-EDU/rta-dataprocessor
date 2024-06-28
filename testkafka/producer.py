import avro.schema
import avro.io
from confluent_kafka import Producer
import time,io

# Schema Avro per AvroMonitoringPoint
avro_schema_str = '''
{
    "type": "record",
    "name": "AvroMonitoringPoint",
    "namespace": "astri.mon.kafka",
    "fields": [
        {"name": "assembly", "type": "string"},
        {"name": "name", "type": "string"},
        {"name": "serial_number", "type": "string"},
        {"name": "timestamp", "type": "long"},
        {"name": "source_timestamp", "type": ["null", "long"]},
        {"name": "units", "type": "string"},
        {"name": "archive_suppress", "type": "boolean"},
        {"name": "env_id", "type": "string"},
        {"name": "eng_gui", "type": "boolean"},
        {"name": "op_gui", "type": "boolean"},
        {"name": "data", "type": {"type": "array", "items": ["double", "int", "long", "string", "boolean"]}}
    ]
}
'''

# Funzione per inviare un messaggio Avro a Kafka
def send_avro_message(topic, message):
    try:
        # Configurazione del produttore Kafka
        producer_conf = {'bootstrap.servers': 'broker:29092'}

        # Crea un produttore Kafka
        producer = Producer(producer_conf)

        # Invia il messaggio al topic specificato
        producer.produce(topic=topic, value=message)
        producer.flush()

        #print(f"Message sent successfully to topic '{topic}': {message}")

    except Exception as e:
        print(f"Error sending message: {e}")



# Esempio dati da inviare
data_to_send = {
    "assembly": "example",
    "name": "point1",
    "serial_number": "SN12345",
    "timestamp": int(time.time() * 1000),  # Esempio di timestamp in millisecondi
    "source_timestamp": None,
    "units": "unit",
    "archive_suppress": False,
    "env_id": "env1",
    "eng_gui": True,
    "op_gui": False,
    "data": [1.0, 2, 3, "four", True]
}

avro_schema = avro.schema.parse(avro_schema_str)

writer = avro.io.DatumWriter(avro_schema)
bytes_io = io.BytesIO()
encoder = avro.io.BinaryEncoder(bytes_io)
writer.write(data_to_send, encoder)
avro_binary_data = bytes_io.getvalue()

# Topic Kafka in cui inviare i messaggi
kafka_topic = 'test_topic'

# Invio del messaggio Avro al topic Kafka
while True:
    time.sleep(0.01)
    send_avro_message(kafka_topic, avro_binary_data)
