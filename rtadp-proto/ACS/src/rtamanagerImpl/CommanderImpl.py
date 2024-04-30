import rtamanager
import rtamanager__POA

from Acspy.Servants.ACSComponent import ACSComponent
from Acspy.Servants.ContainerServices import ContainerServices
from Acspy.Servants.ComponentLifecycle import ComponentLifecycle

from Command import Command

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

    def configure(self, jsonStaticConfiguration, name):
        self._logger.logInfo("Configure Called")
        self._logger.logInfo(f"String: {jsonStaticConfiguration}")
        self.commander = Command(jsonStaticConfiguration, name)
        return
    
    def sendCommand( self, command,  destProcessorName):
        self._logger.logInfo("sendCommand Called")
        self._logger.logInfo(f"command:  {command}")
        self._logger.logInfo(f"target: {destProcessorName}")
        str_command = str(command).lower()
        try:
            self.commander.send_command(str_command, destProcessorName)
        except KeyboardInterrupt:
            print("Command generation stopped.")
        self._logger.logInfo("Command sent")
        return
        
        
        