import rtamanager
import rtamanager__POA

from Acspy.Servants.ACSComponent import ACSComponent
from Acspy.Servants.ContainerServices import ContainerServices
from Acspy.Servants.ComponentLifecycle import ComponentLifecycle

from Supervisor1 import Supervisor1
from Supervisor2 import Supervisor2

class DataProcessorImpl(rtamanager__POA.DataProcessor, ACSComponent, ContainerServices, ComponentLifecycle):
    def __init__(self):
        ACSComponent.__init__(self)
        ContainerServices.__init__(self)

    def initialize(self):
        self._logger = self.getLogger()
        self.supervisor_instance = None
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
        if name == 'RTADP1':
            self.supervisor_instance = Supervisor1(jsonStaticConfiguration, name)
        elif name == 'RTADP2':
            self.supervisor_instance = Supervisor2(jsonStaticConfiguration, name)
        else:
            print("WARNING! Impossible to configure this component, name unknown: possible choises for name are \"RTADP1\" or \"RTADP2\"")
        self._logger.logInfo(f"Supervisor instantiated")
        return

    def start(self):
        self._logger.logInfo("Start Called")
        # TODO: o lo risolvi lanciando thread python oppure aggiornare idle
        try:
            self.supervisor_instance.start()
        except Exception as e:
            print(e)
        self._logger.logInfo("Start exit")
        return