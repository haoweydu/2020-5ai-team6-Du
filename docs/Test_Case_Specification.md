# Test Case Specification For Team 6


# **Table of Contents**

**1) Introduction**

**2) Test Cases: Arduino code and modules**

**3) Test Cases: Web Site**


# 1) Introduction

This document provides the test cases to be carried out for the Arietta Project. Each item to be tested is represented by an individual test case. Each case details the input and expected outputs.

# 2) Test Cases: Arduino code and modules

| Test ID | 2.1 |
| --- | --- |
| Title | Testing DHT22 Module (temperature/humidity) |
| Feature | Measure the temperature/humidity |
| Objective | Confirm that the module is working and that the data is correct |
| Setup | Module connected to the Arduino. Data displayed on the serial monitor. |
| Test Data | Arduino Code/Libraries necessary to use the module |
| Test Actions | 1. Connect the module to Arduino 2. Execute the code 3. Check the data on the serial monitor|
| Expected Results | The temperature measured by the module is displayed correctly |

| Test ID | 2.2 |
| --- | --- |
| Title | Testing KY-038 module (sound) |
| Feature | Returns an analogic value about the volume |
| Objective | Confirm that the module is working and that the data is correct. |
| Setup | Module connected to the Arduino. Data displayed on the serial monitor. |
| Test Data | Arduino Code/Libraries necessary to use the module |
| Test Actions | 1. Connect the module to Arduino 2. Execute the code 3. Check the data on the serial monitor |
| Expected Results | The volume measured by the module is displayed correctly |

# 3) Test Cases: Web Site

| Test ID | 3.1 |
| --- | --- |
| Title | Testing the first version of the website |
| Feature | It displays the measured data |
| Objective | Confirm that the website works  |
| Setup | Site hosted on localhost |
| Test Data | None |
| Test Actions | 1. Using the navbar 2. Checking if the graphs are working |
| Expected Results | The site is working and the charts are displayed|
