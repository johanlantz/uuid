# uuid
Cross platform uuid implementations.

Works with Win32, Apple, Linux and Android. 

Except for Android the implemenatations are trivial. However in Android there is no native uuid generator so we need to do a sort of reflection calling the Java implementation from C. This example relies on a jvm pointer provided by pjsip but it can be replaced with your own instance.
