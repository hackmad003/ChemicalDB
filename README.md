Chemical Database Application
A C++ console application that interfaces with an SQL Server database to provide chemical element information and ionic compound molar mass calculations.
Features
1. Element Information Lookup

Browse available elements in the database
Search elements by chemical symbol (case-insensitive)
Display detailed information including:

Chemical symbol
Element name
Atomic number
Atomic weight
Charge information



2. Ionic Compound Molar Mass Calculator

Browse available cations and anions
Calculate balanced chemical formulas using charge cross-multiplication
Compute total molar mass of ionic compounds
Display step-by-step calculation process
Show charge balancing explanation

Database Schema
The application expects the following tables in a SQL Server database named ChemicalDB:
Elements Table (dbo.Elements)
sql- Symbol (varchar(10)) - Chemical symbol
- Charge (int) - Ionic charge
- Name (varchar(100)) - Element name
- AtomicWeight (decimal(10,4)) - Atomic weight in g/mol
- AtomicNumber (int) - Atomic number
  
Cations Table (dbo.Cations)
sql- Symbol (varchar(10)) - Cation symbol
- Charge (int) - Positive charge
- Name (varchar(100)) - Cation name
- AtomicWeight (decimal(10,4)) - Atomic weight in g/mol
  
Anions Table (dbo.Anions)
sql- Symbol (varchar(10)) - Anion symbol
- Charge (int) - Negative charge
- Name (varchar(100)) - Anion name
- AtomicWeight (decimal(10,4)) - Atomic weight in g/mol
  
Prerequisites
Software Requirements

Microsoft ODBC Driver for SQL Server

Download: Microsoft ODBC Driver
Supports versions 17 or 18


C++ Compiler

MinGW-w64 (recommended)
Or Microsoft Visual C++ Build Tools


SQL Server Instance

SQL Server Express (free) or higher
Must have a database named ChemicalDB



Development Environment

Visual Studio Code (recommended)
C/C++ extension for VS Code
Windows 10/11

Installation & Setup
1. Database Setup
Create the ChemicalDB database and tables:
sql-- Create database
CREATE DATABASE ChemicalDB;
USE ChemicalDB;

-- Create Cations table
CREATE TABLE dbo.Cations (
    Symbol VARCHAR(10) PRIMARY KEY,
    Charge INT NOT NULL,
    Name VARCHAR(100) NOT NULL,
    AtomicWeight DECIMAL(10,4) NOT NULL
);

-- Create Anions table
CREATE TABLE dbo.Anions (
    Symbol VARCHAR(10) PRIMARY KEY,
    Charge INT NOT NULL,
    Name VARCHAR(100) NOT NULL,
    AtomicWeight DECIMAL(10,4) NOT NULL
);

-- Create Elements table
CREATE TABLE dbo.Elements (
    Symbol VARCHAR(10) PRIMARY KEY,
    Charge INT,
    Name VARCHAR(100) NOT NULL,
    AtomicWeight DECIMAL(10,4) NOT NULL,
    AtomicNumber INT
);
2. Sample Data
Insert sample data:
sql-- Sample cations
INSERT INTO dbo.Cations (Symbol, Charge, Name, AtomicWeight)
VALUES 
('H', 1, 'Hydrogen', 1.01),
('Li', 1, 'Lithium', 6.94),
('Na', 1, 'Sodium', 22.99),
('Mg', 2, 'Magnesium', 24.31),
('Ca', 2, 'Calcium', 40.08);

-- Sample anions (add your anion data)
-- Sample elements (add your element data)

3. Compilation
   
Using MinGW:
bashg++ -std=c++17 main.cpp -o chemical_app.exe -lodbc32


Run the application:
bash./chemical_app.exe

Main Menu Options:

Option 1: Look up element information
Option 2: Calculate ionic compound molar mass
Option 3: Exit


connectToDatabase() - Establishes ODBC connection to SQL Server
getElementInfo() - Handles element information lookup
calculateMolarMass() - Performs ionic compound calculations
getIonInfo() - Retrieves cation/anion data from database
listAvailableElements() - Displays available elements
listAvailableIons() - Displays available cations/anions
show_error() - ODBC error handling

Key Features
Charge Balancing Algorithm
The application uses proper chemical charge balancing:
Cross-multiplies charges to determine ion ratios

Error Handling

Comprehensive ODBC error reporting
Input validation
Database connection verification
Graceful handling of missing data

User Experience

Case-insensitive input
Clear step-by-step calculations
Detailed charge balancing explanations
Clean, formatted output

Connection Configuration
The application connects to SQL Server using:

Driver: ODBC Driver 18 for SQL Server
Server: localhost
Database: ChemicalDB
Authentication: Windows Authentication (Trusted Connection)
Security: TrustServerCertificate=Yes

To modify connection settings, update the connection string in connectToDatabase().
Troubleshooting
Common Issues

"Invalid object name" error

Ensure ChemicalDB database exists
Verify table names match exactly (case-sensitive)
Check you're connected to the correct database


ODBC Driver not found

Install Microsoft ODBC Driver for SQL Server
Verify driver version in connection string


Connection failed

Ensure SQL Server is running
Check Windows Authentication permissions
Verify server name (use localhost or actual server name)


Compilation errors

Ensure odbc32.lib is linked
Include proper header files
Use compatible C++ standard (C++17 or later)


License
This project is open source. Feel free to use and modify as needed.
Authors
Created as a chemistry database application for educational purposes.

Note: This application is designed for educational use in chemistry courses and provides a practical example of C++ database integration with real-world chemical calculations.
