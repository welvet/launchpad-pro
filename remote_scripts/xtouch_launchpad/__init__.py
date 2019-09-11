from __future__ import with_statement
import Live
from _Framework.ControlSurface import ControlSurface
from _Framework.DeviceComponent import DeviceComponent
from _Framework.MixerComponent import MixerComponent
from _Framework.SliderElement import SliderElement
from _Framework.TransportComponent import TransportComponent
from _Framework.InputControlElement import *
from _Framework.ButtonElement import ButtonElement
from _Framework.ButtonMatrixElement import ButtonMatrixElement
from _Framework.SessionComponent import SessionComponent
from _Framework.EncoderElement import *
from Launchpad.ConfigurableButtonElement import ConfigurableButtonElement
from _Framework.ControlSurface import get_control_surfaces

class xtouch_launchpad(ControlSurface):

    def __init__(self, c_instance):
        super(xtouch_launchpad, self).__init__(c_instance)
        with self.component_guard():        
            self.__c_instance = c_instance
            self._highlighting_session_component = None 
            self._device_component = None 
            self._device_selection_follows_track_selection = False 

    def can_lock_to_devices(self):
        return False

    def build_midi_map(self, midi_map_handle):        
        for i in range(0, 16):
            Live.MidiMap.forward_midi_note(self.__c_instance.handle(), midi_map_handle, 15, i)

    def receive_midi(self, midi_bytes):
        if midi_bytes[0] == 159:
            for s in get_control_surfaces():
                if hasattr(s, 'activate_pad_control'):
                    s.activate_pad_control(midi_bytes[1], midi_bytes[2] - 36)        
        if midi_bytes[0] == 143:
            for s in get_control_surfaces():
                if hasattr(s, 'deactivate_pad_control'):
                    s.deactivate_pad_control()        
        ControlSurface.receive_midi(self, midi_bytes)           

    def disconnect(self):
        super(xtouch_launchpad, self).disconnect()


def create_instance(c_instance):
    return xtouch_launchpad(c_instance)
