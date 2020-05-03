# Marconi App

## Architecture/Design Document

### Table of Contents

### Change History
Version: 1.0

Modifier: Du, Gilt, Sansone

Date: 1/11/2019

Description of Change: first setting of the project;

______________________________________________________

Version: 1.0

Modifier: Du, Gilt, Sansone

Date: 1/02/2020

Description of Change: first creation of the project design;

## 1 Introduction

The purpose of the architecture/design document is to explain the organization of the code. 

A well-written architecture document will make it easier for new programmers to become familiar with the code.

The architecture/design document should identify major system components and describe their static attributes and dynamic patterns of interaction.



The purpose of this document is to describe the architecture and design of Arietta project in a way that addresses the interests and concerns of all major stakeholders. 

For this application the major stakeholders are:

Users and the customer – they want assurances that the architecture will provide for system functionality and exhibit desirable non-functional quality requirements such as usability, reliability, etc.

Developers – they want an architecture that will minimize complexity and development effort.

Maintenance Programmers – they want assurance that the system will be easy to evolve and maintain on into the future.

Project Manager – the project manager is responsible for assigning tasks and coordinating development work. 

He or she wants an architecture that divides the system into components of roughly equal size and complexity that can be developed simultaneously with minimal dependencies. 

For this to happen, the modules need well-defined interfaces. 

Also, because most individuals specialize in a particular skill or technology, modules should be designed around specific expertise. 

For example, all UI logic might be encapsulated in one module. 

Another might have all business logic.


The architecture and design for a software system is complex and individual stakeholders often have specialized interests. 

There is no one diagram or model that can easily express a system’s architecture and design. 

For this reason, software architecture and design is often presented in terms of multiple views or perspectives [IEEE Std. 1471]. 

Here the architecture of Arietta project application is described from 4 different perspectives [1995 Krutchen]:

Logical View – major components, their attributes and operations. This view also includes relationships between components and their interactions. When doing OO design, class diagrams and sequence diagrams are often used to express the logical view.

Process View – the threads of control and processes used to execute the operations identified in the logical view.

Development View – how system modules map to development organization. 

Use Case View – the use case view is used to both motivate and validate design activity. At the start of design the requirements define the functional objectives for the design. Use cases are also used to validate suggested designs. 

It should be possible to walk through a use case scenario and follow the interaction between high-level components. 

The components should have all the necessary behavior to conceptually execute a use case.



### 2 Design Goals
There is no absolute measure for distinguishing between good and bad design. 

The value of a design depends on stakeholder priorities. 

For example, depending on the circumstances, an efficient design might be better than a maintainable one, or vise versa. 

Therefore, before presenting a design it is good practice to state the design priorities. 

The design that is offered will be judged according to how well it satisfies the stated priorities.

The design priorities for the Arietta project application are:

-Simple and easy to interpret design  

-A design that has everything you need 

### 3 System Behavior
The use case view is used both to guide the design phase and to validate the output of the design phase. 

The description of the architecture presented here begins with a review of the behavior of the system and provides for setting the stage for the description of the architecture that follows.

For a more detailed account of software requirements, see the requirements document.

Once the webiste is open, the user should see all the section and all the data we need.

### 4 Logical View
The logical view describes the main functional components of the system.

This includes modules, the static relationships between modules, and their dynamic patterns of interaction.

In this section the modules of the system are first expressed in terms of high level components (architecture) and progressively refined into more detailed components and eventually classes with specific attributes and operations.

#### 5 High-Level Design (Architecture)
The high-level view or architecture consists of 1 major components:

The Database is a central repository for data on temperature and audio.


