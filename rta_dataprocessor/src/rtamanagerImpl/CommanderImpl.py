import rtamanager
import rtamanager__POA

from Acspy.Servants.ACSComponent import ACSComponent
from Acspy.Servants.ContainerServices import ContainerServices
from Acspy.Servants.ComponentLifecycle import ComponentLifecycle


class CommanderImpl(rtamanager__POA.Commander, ACSComponent, ContainerServices, ComponentLifecycle):
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

    def configure(self, jsonStaticConfiguration):
        self._logger.logInfo("Configure Called")
        self._logger.logInfo(f"String: {jsonStaticConfiguration}")
        return
    
    def sendCommand( self, command,  destProcessorName):
        self._logger.logInfo("sendCommand Called")
        self._logger.logInfo(f"target: {destProcessorName}")
        self._logger.logInfo(f"command:  {command}")
        return
        
        
        