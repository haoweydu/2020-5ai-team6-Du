# System Documentation Team 6

**Team Members**

Gilt Andres, Sansone Robert.

# Contents

1) Introduction
2) Website
3) Setup of Arduino/Modules
4) System Maintenance 

# 1. Introduction

The goal of our project is to offer the school a system capable of measuring data about the temperature, humidity and sound intensity inside the building. The data will be stored inside a database and displayed on a website.

# 2. Website

To test the website it's possible to host it on localhost using XAMPP. If the project will be a success the school could host it on a server with its own domain and make it accessible to the school administration.
It will have different sections (one for the temperature/humidity and one dedicated to the sound intensity), both accessible through a navbar.


# 3. Setup of Arduino/Modules

## 3.1 Required Components

In order to get the required data you must use two main Arduino's modules.

- DHT22 for the temperature/humidity
- KY-038 for the sound intensity (currently looking for a better module)

## 3.2 Using the modules
To start getting the data you must first connect the modules to the Arduino and then launch the script that we will give to the school.
Once the script has been launched the modules will start getting the data and sending it to a database. 
To see the stats about the temperature/humidity and the sound volume the user has to connect to a website, which will get the data from the database and display it.

**Connecting the DHT22 module to the Arduino**
1. Connect the digital output pin of the module to the digital  pin on the Arduino board.
2. Connect the ground pin of the module to the ground pin of the board.
3. Connect the VCC pin of the module to the 5V pin on the board

**Connecting the KY-038 module to the Arduino**
1. Connect the analog output pin of the module to the analog pin on the Arduino board.
2. Connect the ground pin of the module to the ground pin of the board.
3. Connect the VCC pin of the module to the 5V pin on the board

