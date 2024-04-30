import rtamanager
import rtamanager__POA

from Acspy.Servants.ACSComponent import ACSComponent
from Acspy.Servants.ContainerServices import ContainerServices
from Acspy.Servants.ComponentLifecycle import ComponentLifecycle

from ProcessMonitoring import MonitoringConsumer

class MonitoringImpl(rtamanager__POA.Monitoring, ACSComponent, ContainerServices, ComponentLifecycle):
    def __init__(self):
        ACSComponent.__init__(self)
        ContainerServices.__init__(self)

    def initialize(self):
        self._logger = self.getLogger()
        self._logger.logInfo(f"[initialize]")

    def execute(self):
        self._logger.logInfo(f"[execute]")

    def cleanUp(self):
        self._logger.logInfo(f"[cleanUp]")

    def aboutToAbort(self):
        self._logger.logInfo(f"[aboutToAbort]")

    def configure(self, jsonStaticConfiguration, name):
        self._logger.logInfo("Configure Called")
        self._logger.logInfo(f"String: {jsonStaticConfiguration}")
        self.monitoring = MonitoringConsumer(jsonStaticConfiguration, name)
        return
    
    def start(self):
        self._logger.logInfo("start Called")
        try:
            self.monitoring.receive_and_decode_messages()
        except KeyboardInterrupt:
            print("Monitoring stopped.")
        self._logger.logInfo("Monitoring stopped")
        return
        
        
        