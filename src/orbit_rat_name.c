#include <usb_names.h>

// #define PRODUCT_NAME                {'O','r','b','i','t','R','a','t'}
// #define PRODUCT_NAME_LEN        8

// struct usb_string_descriptor_struct usb_string_product_name = {
//   2 + PRODUCT_NAME_LEN * 2,
//   3,
//   PRODUCT_NAME
// };

// GreyHelix.com:OrbitRatV1

struct usb_string_descriptor_struct usb_string_serial_number = {
    2 + 24 * 2,
    3,
    {'G', 'r', 'e','y','H','e','l','i', 'x','.','c','o', 'm',':','O','r','b','i','t','R','a','t','V','1'}
};
