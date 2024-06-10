import logging

class WorkerLogger:
    SYSTEM_LEVEL_NUM = 25  # Definisce il valore del nuovo livello SYSTEM

    def __init__(self, logger_name='my_logger', log_file='my_log_file.log', level=logging.DEBUG):
        # Creare un logger
        self.logger = logging.getLogger(logger_name)
        self.logger.setLevel(level)
        
        # Creare un formatter personalizzato
        class CustomFormatter(logging.Formatter):
            def format(self, record):
                # Verifica se 'extra_info' è presente nel record
                if not hasattr(record, 'extra_info'):
                    record.extra_info = ''  # Default value if not present
                return super().format(record)
        
        self.formatter = CustomFormatter('%(asctime)s - %(levelname)s - %(extra_info)s - "%(message)s"')
        
        # Impostare il file handler
        self.file_handler = logging.FileHandler(log_file, mode='w')
        self.file_handler.setLevel(level)
        self.file_handler.setFormatter(self.formatter)
        
        # Aggiungere il file handler al logger
        self.logger.addHandler(self.file_handler)
        
        # Aggiungere il nuovo livello SYSTEM
        logging.addLevelName(self.SYSTEM_LEVEL_NUM, 'SYSTEM')

        # Aggiungere il metodo system al logger
        def system(self, message, *args, **kws):
            if self.isEnabledFor(WorkerLogger.SYSTEM_LEVEL_NUM):
                self._log(WorkerLogger.SYSTEM_LEVEL_NUM, message, args, **kws)

        logging.Logger.system = system

    def debug(self, msg, extra=''):
        self.logger.debug(msg, extra={'extra_info': extra})
    
    def info(self, msg, extra=''):
        self.logger.info(msg, extra={'extra_info': extra})
    
    def warning(self, msg, extra=''):
        self.logger.warning(msg, extra={'extra_info': extra})
    
    def error(self, msg, extra=''):
        self.logger.error(msg, extra={'extra_info': extra})
    
    def critical(self, msg, extra=''):
        self.logger.critical(msg, extra={'extra_info': extra})

    def system(self, msg, extra=''):
        self.logger.system(msg, extra={'extra_info': extra})
