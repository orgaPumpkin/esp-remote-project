![profiles.png](https://raw.githubusercontent.com/orgaPumpkin/esp-remote-project/refs/heads/master/profiles.png)
![remote.png](https://raw.githubusercontent.com/orgaPumpkin/esp-remote-project/refs/heads/master/remote.png)
![edit.png](https://raw.githubusercontent.com/orgaPumpkin/esp-remote-project/refs/heads/master/edit.png)
![edit.png](https://raw.githubusercontent.com/orgaPumpkin/esp-remote-project/refs/heads/master/setup.png)

![wakatime](https://img.shields.io/endpoint?url=https%3A%2F%2Fwaka.hackclub.com%2Fapi%2Fcompat%2Fshields%2Fv1%2FU07QBRYGVHC%2Fall_time%2Fproject%253Aesp-remote)

# What Does It Do?
The esp-remote is a program for the esp8266 microcontroller that can transform any IR (infrared)
remote controlled device (such as air conditioners, tvs and projectors) into a Wi-Fi connected device.
The device can be controlled from anywhere with a web page.

This esp-remote records IR messages from the device's original remote,
and then uses an algorithm to analyze the IR protocol of the device.

The esp-remote can later construct and send custom messages based on the learned protocol to the device.

# How Was It Developed?
The first step of developing the esp-remote was a prototype that was only able to send IR messages in
a specific protocol that was hard-coded.

After that I started writing the final product in python,
but then I reached memory and speed limits of the esp8266 platform that wasn't possible to work around.

In the end, I reverted to coding in c++, which was harder but also way more efficient.

The only step of the process that was recorded in the Hackatime is a part of the last step,
which means that in total, the amount of time spent might be more than double the time recorded.

# [@mouseylicense](https://github.com/mouseylicense)'s Contribution
@mouseylicense helped me with the UI.
Without his help, the website's UI would have looked more like a CLI than a website.

# More Info And How To Use
More info can be found in the [main readme](https://github.com/orgaPumpkin/esp-remote-project/blob/master/readme.md).
