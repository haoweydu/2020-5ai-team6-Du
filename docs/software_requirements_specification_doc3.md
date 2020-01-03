---

<h1 align = "center">
Software Requirements Specification<br>
For<br>
Team 6
</h1>
<p align = "center">
December 22, 2019
Version 1
</p>

### 1 Introduction
#### _1.1 OVERVIEW_ 
The product is a system that is able to measure the temperature, the humidity and the noise intensity from a determined area and to store the data into a database.
  
#### _1.2 GOALS AND OBJECTIVES_
Create an informatic system that will help the school keep track of the temperature, humidity and noise intensity.

#### _1.3 SCOPE_
We started the project for a school job, the purpose of which is to improve our skills in the informatic field and also teach us how to manage a real project by ourselves. 

#### _1.4 DEFINITIONS_
Arietta – the product that is being described here; the software system specified in this document.

Project – activities that will lead to the production of the Arietta project.

Client – the person or organization for which this Arietta project is being built. 

User – the person or persons who will actually interact with the Arietta Project.

Use case – describes a goal-oriented interaction between the system and an actor. A use case may define several variants called 
scenarios that result in different paths through the use case and usually different outcomes.

Scenario – one path through a user case

Actor – user or other software system that receives value from a user case.

Developer – the person or organization developing the system, also sometimes called the supplier.

Stakeholder – anyone with an interest in the project and its outcomes. This includes clients, customers, users, developers, testers, managers and executives.

### 2 General Design Constraints
#### _2.1 Arietta project environment_
The idea of our project is to use an Arduino and modules of temperature,humidity and sound to collect data relating to the sorrounding area and send it into a database. Then we will use an html web page to see suitable graphs

#### _2.2 User Characteristics_
The "Arietta" project is intended for the School and the students.The school will use the website to see all the graphs displaying data regard temperature, humidity and noise intensity of the classrooms.

#### _2.3 Mandated Costraints_
The project must be delivered with all its relative documentation and must be completed within the deadline.

### 3 Nonfunctional requirements
#### _3.1	Operational Requirements_
Usability : It'll be simple for everyone to read the graphs
#### _3.2  Performance Requirements_
Maintainability : Changes of the Arduino code won't alterate the graphs' accuracy.
#### _3.3 Security Requirements_
Someone could connect with the arduino to steal the data and send it into another database.
#### _3.4 Documentation and training_
The Arietta Project will be delivered to users as a download without documentation or training.  A user guide and system documentation will be provided to project stakeholders.
#### _3.5 External User Interface_
##### _3.5.1 User Interface_
The user interface will be eye-catching and visually appealing. The interface will be intuitive.  As a mobile app it will be streamlined and simple to use.  No training will be provided and it is expected that 95% of users will be able to use the app without any training.
##### _3.5.2 Software Interface_
