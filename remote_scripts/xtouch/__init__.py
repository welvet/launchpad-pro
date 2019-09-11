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


class CustomDeviceComponent(DeviceComponent):
	def __init__(self, log_message):
		super(CustomDeviceComponent, self).__init__()
		self.log_message = log_message
		
	def _current_bank_details(self):
		device = self._device		

		def param(name):
			for param in device.parameters:
				if param.name == name:
					return param
			return None

		#i = 0
		#for param in device.parameters:
		#	self.log_message("Param " + str(i) + " " + str(param.name))
		#	i += 1		

		return ("Hack 2", [
			param("S Start"),
			param("S Length"),
			param("Filter Freq"),
			param("Filter Res"),
			param("Filter Type"),
			param("Transpose"),
			param("Pan"),
			param("Volume"),			
		])

class xtouch(ControlSurface):

	def __init__(self, c_instance):
		super(xtouch, self).__init__(c_instance)
		with self.component_guard():
			self.__c_instance = c_instance

			self._highlighting_session_component = None 
			self._device_selection_follows_track_selection = False 
			self._device_component = None 

			self._pad_control_device = None
			self._track_control_device = None

			self._should_update_tracks = 0

			self._map_modes = Live.MidiMap.MapMode
			self.current_track_offset = 0
			self.current_scene_offset = 0
						
			self.mixer = MixerComponent(128, 24)
						
			num_of_tracks = len(self.song().tracks)
			for index in range(num_of_tracks):
				self.song().tracks[index].view.add_selected_device_listener(self._reload_track_control)

			self._reload_track_control()			

	def best_of_parameter_bank(self):
		return []

	def _do_deactivate_pad_control(self):
		with self.component_guard():
			if (self._pad_control_device is not None):
				device_controls = (None, None, None, None, None, None, None, None)

				self._pad_control_device.set_parameter_controls(tuple(device_controls))
				self._pad_control_device.set_lock_to_device("", self.actual_device)
				self._pad_control_device.set_device(None)

				self._pad_control_device = None

	def activate_pad_control(self, track, pad):
		self._do_deactivate_pad_control()

		with self.component_guard():
			self._deactivate_track_control()

			track_id = track + 8
			tracks = self.mixer.song().tracks

			if track_id < len(tracks):
				track = tracks[track_id]
			
				if (len(track.devices) > 0):
					drumpad = track.devices[0].drum_pads[36 + pad]
					if len(drumpad.chains) > 0:
						if len(drumpad.chains[0].devices) > 0:
							self.actual_device = drumpad.chains[0].devices[0]

							def log(msg):
								self.log_message(msg)
									
							self._pad_control_device = CustomDeviceComponent(log)
							device_controls = (
								EncoderElement(MIDI_CC_TYPE, 10, 1, self._map_modes.absolute),
								EncoderElement(MIDI_CC_TYPE, 10, 2, self._map_modes.absolute),
								EncoderElement(MIDI_CC_TYPE, 10, 3, self._map_modes.absolute),
								EncoderElement(MIDI_CC_TYPE, 10, 4, self._map_modes.absolute),
								EncoderElement(MIDI_CC_TYPE, 10, 5, self._map_modes.absolute),
								EncoderElement(MIDI_CC_TYPE, 10, 6, self._map_modes.absolute),
								EncoderElement(MIDI_CC_TYPE, 10, 7, self._map_modes.absolute),
								EncoderElement(MIDI_CC_TYPE, 10, 8, self._map_modes.absolute),
							)

							self.actual_device.playback_mode = 0
							self.actual_device.parameters[31].value = 1.0	

							self._pad_control_device.set_device(self.actual_device)
							self._pad_control_device.set_parameter_controls(tuple(device_controls))
							self._pad_control_device.set_lock_to_device("lock", self.actual_device)
							self.log_message("AA " + str(self._pad_control_device._current_bank_details()))

	def deactivate_pad_control(self):
		with self.component_guard():
			self._do_deactivate_pad_control()
			self._reload_track_control()

	def _activate_track_control(self):
		if self._pad_control_device is not None:
			return

		if (len(self.mixer.selected_strip()._track.devices) > 0):
			devices = self.mixer.selected_strip()._track.devices
			self.actual_device = devices[0]
					
			self._track_control_device = DeviceComponent()
			device_controls = (
				EncoderElement(MIDI_CC_TYPE, 10, 1, self._map_modes.absolute),
				EncoderElement(MIDI_CC_TYPE, 10, 2, self._map_modes.absolute),
				EncoderElement(MIDI_CC_TYPE, 10, 3, self._map_modes.absolute),
				EncoderElement(MIDI_CC_TYPE, 10, 4, self._map_modes.absolute),
				EncoderElement(MIDI_CC_TYPE, 10, 5, self._map_modes.absolute),
				EncoderElement(MIDI_CC_TYPE, 10, 6, self._map_modes.absolute),
				EncoderElement(MIDI_CC_TYPE, 10, 7, self._map_modes.absolute),
				EncoderElement(MIDI_CC_TYPE, 10, 8, self._map_modes.absolute),
			)

			self._track_control_device.set_device(self.actual_device)
			self._track_control_device.set_parameter_controls(tuple(device_controls))
			self._track_control_device.set_lock_to_device("lock", self.actual_device)

	def _deactivate_track_control(self):
		if (self._track_control_device is not None):
			device_controls = (None, None, None, None, None, None, None, None)

			self._track_control_device.set_parameter_controls(tuple(device_controls))
			self._track_control_device.set_lock_to_device("", self.actual_device)
			self._track_control_device.set_device(None)

			self._track_control_device = None

	def _reload_track_control(self, value = None): 
		self._deactivate_track_control()
		self._activate_track_control()  

	def _highlight_tracks(self):
		j = [0]		
		def update_next_track(selected):
			if selected:				
				self.__c_instance.send_midi((10 | 0x90, 8 + j[0], 127))
			else:
				self.__c_instance.send_midi((10 | 0x80, 8 + j[0], 0))				
			j[0] += 1

		selected_track = self.song().view.selected_track
		for i in [0, 1, 2, 3, 4, 5, 6, 7, 17, 18]:		
			track_selected = False
			if i < len(self.song().tracks):
				if self.song().tracks[i] == selected_track:
					track_selected = True

			update_next_track(track_selected)

		update_next_track(self.song().return_tracks[0] == selected_track)
		update_next_track(self.song().return_tracks[1] == selected_track)
		update_next_track(self.song().master_track == selected_track)
			

	def _on_selected_track_changed(self):
		ControlSurface._on_selected_track_changed(self)
		self._display_reset_delay = 0
		self._reload_track_control()
		self._should_update_tracks = 3

	def build_midi_map(self, midi_map_handle):        
		ControlSurface.build_midi_map(self, midi_map_handle)
		for i in range(0, 16):
			Live.MidiMap.forward_midi_note(self.__c_instance.handle(), midi_map_handle, 10, 8 + i)

	def receive_midi(self, midi_bytes):
		ControlSurface.receive_midi(self, midi_bytes)
		if (midi_bytes[0] == 10 | 0x90) or (midi_bytes[0] == 10 | 0x80):
			if midi_bytes[1] >= 8 and midi_bytes[1] <= 24:
				track_id = midi_bytes[1] - 8
				if (track_id < 8):
					self.song().view.selected_track = self.song().tracks[track_id]
				elif (track_id < 10):
					self.song().view.selected_track = self.song().tracks[9 + track_id]
				elif (track_id < 12):
					self.song().view.selected_track = self.song().return_tracks[track_id - 10]
				else:
					self.song().view.selected_track = self.song().master_track

				self._should_update_tracks = 2

	def update_display(self):
		ControlSurface.update_display(self)		
		if self._should_update_tracks == 1:
			self._highlight_tracks()
			self._should_update_tracks = 0

		if self._should_update_tracks > 1:
			self._should_update_tracks -= 1


	def disconnect(self):
		super(xtouch, self).disconnect()


def create_instance(c_instance):
	return xtouch(c_instance)
