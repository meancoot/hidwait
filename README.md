hidwait
=======

Simple OSX CLI utility to run a command when a button is pressed on a gamepad


    Usage %s --command=command_to_run [options]
      (Option)               (Description)                  (Default)
      --device=device_name   Name of HID device to monitor  PLAYSTATION(R)3 Controller
      --button=button_index  Button number to monitor       16
      --cancel=seconds       If button is held longer than  1
                             this it is ignored.
````

By default it will wait for the PS button on a PlayStation 3 controller. The --cancel argument is used so that it won't pick up when you hold the PS button down to turn off the controller.
