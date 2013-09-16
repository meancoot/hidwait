#include <IOKit/hid/IOHIDManager.h>
#include <sys/time.h>

struct
{
   const char* command;
   const char* device;
   uint32_t    button;
   uint64_t    cancel;
}  params;

typedef struct option_t
{
   const char* name;
   const char* scan;
   void*       value;
}  option_t;

static option_t options[] =
{
   { "--command=",   0,       &params.command   },
   { "--device=",    0,       &params.device    },
   { "--button=",    "%u",    &params.button    },
   { "--cancel=",    "%ull",  &params.cancel    },
   { 0,              0,       0                 }
};

void print_usage_and_exit(const char* argv0)
{
   printf("Usage %s --command=command_to_run\n\n", argv0);
   printf("  (Option)               (Description)                  (Default)\n");
   printf("  --device=device_name   Name of HID device to monitor  PLAYSTATION(R)3 Controller\n");
   printf("  --button=button_index  Button number to monitor       16\n");
   printf("  --cancel=seconds       If button is held longer than  1\n");
   printf("                         this it is ignored.\n");
   printf("\n\n");

   exit(1);
}

void parse_command_line(uint32_t argc, char* argv[])
{
   params.device = "PLAYSTATION(R)3 Controller";
   params.button = 16;
   params.cancel = 1;

   for (char** arg = &argv[1]; *arg; arg ++)
   {
      bool found_it = false;

      for (option_t* option = options; option && option->name; option ++)
      {
         if (strstr(*arg, option->name) == *arg)
         {
            if (option->scan && sscanf(*arg, option->scan, option->value) == 1)
            {
               found_it = true;
               break;
            }
            else if (!option->scan)
            {
               *(const char**)option->value = *arg + strlen(option->name);
               found_it = true;
               break;
            }
         }
      }

      if (!found_it)
      {
         fprintf(stderr, "Invalid argument: %s\n\n", *arg);
         print_usage_and_exit(argv[0]);
      }
   }

   if (!params.command)
   {
      fprintf(stderr, "No --command= argument given.\n\n");
      print_usage_and_exit(argv[0]);
   }

   printf("device=%s\nbutton=%u\ncancel=%llu\ncommand=%s\n", params.device, params.button, 
                                                             params.cancel, params.command);
}

struct
{
   uint32_t run_command;
   uint32_t is_pressed;
   uint64_t pressed_time;
}  state;

static uint64_t get_milliseconds()
{
   struct timeval time;
   gettimeofday(&time, NULL);
   return time.tv_sec * 1000 + time.tv_usec / 1000;
}

static void hid_device_input_callback(void* context, IOReturn result, void* sender, IOHIDValueRef value)
{
   struct hidpad_connection* connection = context;

   IOHIDElementRef element = IOHIDValueGetElement(value);
   if (IOHIDElementGetType(element)      == kIOHIDElementTypeInput_Button &&
       IOHIDElementGetUsagePage(element) == kHIDPage_Button &&
       IOHIDElementGetUsage(element)     == params.button + 1)
   {
      if (IOHIDValueGetIntegerValue(value))
      {
         state.is_pressed = 1;
         state.pressed_time = get_milliseconds();
      }
      else if (state.is_pressed == 1)
      {
         state.is_pressed = 0;

         if ((get_milliseconds() - state.pressed_time) < (params.cancel * 1000))
         {
            state.run_command = 1;
            CFRunLoopStop(CFRunLoopGetCurrent());
         }
      }
   }
}

static void hid_manager_device_attached(void* context, IOReturn result, void* sender, IOHIDDeviceRef device)
{
   CFStringRef device_name = IOHIDDeviceGetProperty(device, CFSTR(kIOHIDProductKey));

   if (device_name)
   {
      char buffer[1024];
      memset(buffer, 0, sizeof(buffer));
      CFStringGetCString(device_name, buffer, 1024, kCFStringEncodingUTF8);

      if (strcmp(buffer, params.device) == 0)
      {
         IOHIDDeviceOpen(device, kIOHIDOptionsTypeNone);
         IOHIDDeviceScheduleWithRunLoop(device, CFRunLoopGetCurrent(), kCFRunLoopCommonModes);
         IOHIDDeviceRegisterInputValueCallback(device, hid_device_input_callback, 0);
      }
   }
}

static void append_matching_dictionary(CFMutableArrayRef array, uint32_t page, uint32_t use)
{
   CFMutableDictionaryRef matcher = CFDictionaryCreateMutable(kCFAllocatorDefault, 0, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);

   CFNumberRef pagen = CFNumberCreate(kCFAllocatorDefault, kCFNumberIntType, &page);
   CFDictionarySetValue(matcher, CFSTR(kIOHIDDeviceUsagePageKey), pagen);
   CFRelease(pagen);

   CFNumberRef usen = CFNumberCreate(kCFAllocatorDefault, kCFNumberIntType, &use);
   CFDictionarySetValue(matcher, CFSTR(kIOHIDDeviceUsageKey), usen);
   CFRelease(usen);

   CFArrayAppendValue(array, matcher);
   CFRelease(matcher);
}

int main(int argc, char* argv[])
{
   parse_command_line(argc, argv);

   IOHIDManagerRef hid_manager = IOHIDManagerCreate(kCFAllocatorDefault, kIOHIDOptionsTypeNone);

   CFMutableArrayRef matcher = CFArrayCreateMutable(kCFAllocatorDefault, 0, &kCFTypeArrayCallBacks);
   append_matching_dictionary(matcher, kHIDPage_GenericDesktop, kHIDUsage_GD_Joystick);
   IOHIDManagerSetDeviceMatchingMultiple(hid_manager, matcher);
   CFRelease(matcher);

   IOHIDManagerRegisterDeviceMatchingCallback(hid_manager, hid_manager_device_attached, 0);
   IOHIDManagerScheduleWithRunLoop(hid_manager, CFRunLoopGetMain(), kCFRunLoopCommonModes);

   IOHIDManagerOpen(hid_manager, kIOHIDOptionsTypeNone);

   do
   {
      CFRunLoopRun();

      if (state.run_command)
      {
         system(params.command);
      }
   }  while(state.run_command);

   IOHIDManagerUnscheduleFromRunLoop(hid_manager, CFRunLoopGetCurrent(), kCFRunLoopCommonModes);
   IOHIDManagerClose(hid_manager, kIOHIDOptionsTypeNone);
   CFRelease(hid_manager);

   return 0;
}
