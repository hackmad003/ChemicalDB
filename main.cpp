#include <iostream>
#include <windows.h>
#include <sql.h>
#include <sqlext.h>
#include <string>
#include <iomanip>
#include <limits>
#include <cstdio>
#include <vector>
using namespace std;

// Function to report ODBC errors
void show_error(SQLHANDLE handle, SQLSMALLINT type) {
    SQLSMALLINT i = 0;
    SQLINTEGER native;
    SQLCHAR state[7];
    SQLCHAR text[256];
    SQLSMALLINT len;
    SQLRETURN ret;

    printf("Error occurred:\n");
    
    do {
        ret = SQLGetDiagRec(type, handle, ++i, state, &native, text, sizeof(text), &len);
        if (SQL_SUCCEEDED(ret)) {
            printf("%s:%d:%d:%s\n", state, i, (int)native, text);
        }
    } while (ret == SQL_SUCCESS);
}

// Function to connect to database
bool connectToDatabase(SQLHENV &env, SQLHDBC &dbc) {
    SQLRETURN ret;

    // Initialize ODBC environment
    ret = SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &env);
    if (!SQL_SUCCEEDED(ret)) {
        printf("Failed to allocate environment handle.\n");
        return false;
    }
    
    // Set ODBC version
    ret = SQLSetEnvAttr(env, SQL_ATTR_ODBC_VERSION, (SQLPOINTER)SQL_OV_ODBC3, 0);
    if (!SQL_SUCCEEDED(ret)) {
        printf("Failed to set ODBC version.\n");
        show_error(env, SQL_HANDLE_ENV);
        SQLFreeHandle(SQL_HANDLE_ENV, env);
        return false;
    }
    
    // Allocate connection handle
    ret = SQLAllocHandle(SQL_HANDLE_DBC, env, &dbc);
    if (!SQL_SUCCEEDED(ret)) {
        printf("Failed to allocate connection handle.\n");
        show_error(env, SQL_HANDLE_ENV);
        SQLFreeHandle(SQL_HANDLE_ENV, env);
        return false;
    }
    
    // Connection string
    SQLCHAR* conn_str = (SQLCHAR*)"DRIVER={ODBC Driver 18 for SQL Server};"
                               "SERVER=localhost;"
                               "DATABASE=ChemicalDB;"
                               "Trusted_Connection=Yes;"
                               "TrustServerCertificate=Yes;";
    
    // Connect to the database
    printf("Connecting to database...\n");
    ret = SQLDriverConnect(dbc, NULL, conn_str, SQL_NTS, NULL, 0, NULL, SQL_DRIVER_NOPROMPT);
    if (!SQL_SUCCEEDED(ret)) {
        printf("Failed to connect to database.\n");
        show_error(dbc, SQL_HANDLE_DBC);
        SQLFreeHandle(SQL_HANDLE_DBC, dbc);
        SQLFreeHandle(SQL_HANDLE_ENV, env);
        return false;
    }
    
    printf("Successfully connected to the database!\n");
    
    // Verify database structure
    printf("Verifying table structure...\n");
    SQLHSTMT stmt = SQL_NULL_HSTMT;
    
    // Allocate statement handle
    ret = SQLAllocHandle(SQL_HANDLE_STMT, dbc, &stmt);
    if (!SQL_SUCCEEDED(ret)) {
        printf("Warning: Could not verify table structure.\n");
        return true; // Continue anyway
    }
    
    // Check Elements table structure
    ret = SQLExecDirect(stmt, (SQLCHAR*)"SELECT TOP 1 Symbol, Charge, Name, AtomicWeight, AtomicNumber FROM dbo.Elements", SQL_NTS);
    if (!SQL_SUCCEEDED(ret)) {
        printf("Warning: Elements table structure doesn't match expected columns.\n");
        show_error(stmt, SQL_HANDLE_STMT);
    } else {
        printf("Elements table structure verified.\n");
    }
    
    // Check Cations table structure
    SQLFreeStmt(stmt, SQL_CLOSE);
    ret = SQLExecDirect(stmt, (SQLCHAR*)"SELECT TOP 1 Symbol, Charge, Name, AtomicWeight FROM dbo.Cations", SQL_NTS);
    if (!SQL_SUCCEEDED(ret)) {
        printf("Warning: Cations table structure doesn't match expected columns.\n");
        show_error(stmt, SQL_HANDLE_STMT);
    } else {
        printf("Cations table structure verified.\n");
    }
    
    // Check Anions table structure
    SQLFreeStmt(stmt, SQL_CLOSE);
    ret = SQLExecDirect(stmt, (SQLCHAR*)"SELECT TOP 1 Symbol, Charge, Name, AtomicWeight FROM dbo.Anions", SQL_NTS);
    if (!SQL_SUCCEEDED(ret)) {
        printf("Warning: Anions table structure doesn't match expected columns.\n");
        show_error(stmt, SQL_HANDLE_STMT);
    } else {
        printf("Anions table structure verified.\n");
    }
    
    SQLFreeHandle(SQL_HANDLE_STMT, stmt);
    return true;
}

// Function to list available elements
void listAvailableElements(SQLHDBC dbc) {
    SQLHSTMT stmt = SQL_NULL_HSTMT;
    SQLRETURN ret;
    
    // Allocate statement handle
    ret = SQLAllocHandle(SQL_HANDLE_STMT, dbc, &stmt);
    if (!SQL_SUCCEEDED(ret)) {
        printf("Failed to allocate statement handle.\n");
        show_error(dbc, SQL_HANDLE_DBC);
        return;
    }
    
    // Prepare SQL query
    string query = "SELECT Symbol, Name FROM dbo.Elements ORDER BY AtomicNumber";
    
    ret = SQLExecDirect(stmt, (SQLCHAR*)query.c_str(), SQL_NTS);
    if (!SQL_SUCCEEDED(ret)) {
        printf("Failed to execute query.\n");
        show_error(stmt, SQL_HANDLE_STMT);
        SQLFreeHandle(SQL_HANDLE_STMT, stmt);
        return;
    }
    
    // Variables to hold data
    SQLCHAR symbol[50] = {0};
    SQLCHAR name[100] = {0};
    SQLLEN lenSymbol, lenName;
    
    // Display header
    printf("\nAvailable Elements:\n");
    printf("------------------------\n");
    printf("%-10s | %s\n", "Symbol", "Name");
    printf("------------------------\n");
    
    // Fetch and display results
    while (SQL_SUCCEEDED(ret = SQLFetch(stmt))) {
        // Get column data
        SQLGetData(stmt, 1, SQL_C_CHAR, symbol, sizeof(symbol), &lenSymbol);
        SQLGetData(stmt, 2, SQL_C_CHAR, name, sizeof(name), &lenName);
        
        // Print data
        printf("%-10s | %s\n", 
            (lenSymbol == SQL_NULL_DATA ? "NULL" : (char*)symbol),
            (lenName == SQL_NULL_DATA ? "NULL" : (char*)name));
    }
    
    printf("------------------------\n");
    
    // Clean up
    SQLFreeHandle(SQL_HANDLE_STMT, stmt);
}

// Function to get element information by symbol
void getElementInfo(SQLHDBC dbc) {
    SQLHSTMT stmt = SQL_NULL_HSTMT;
    SQLRETURN ret;
    string symbol, symbolDisplay;

    printf("\n=== Element Information Lookup ===\n");
    
    // Show available elements
    listAvailableElements(dbc);
    
    printf("Enter element symbol (e.g., H, He, Li): ");
    cin >> symbol;

    // Store original casing
    symbolDisplay = symbol;
    
    // Convert to uppercase for consistent matching
    for (char &c : symbol) {
        c = toupper(c);
    }

    // Allocate statement handle
    ret = SQLAllocHandle(SQL_HANDLE_STMT, dbc, &stmt);
    if (!SQL_SUCCEEDED(ret)) {
        printf("Failed to allocate statement handle.\n");
        show_error(dbc, SQL_HANDLE_DBC);
        return;
    }

    // Use a direct SQL query with the symbol value inline
    string query = "SELECT Symbol, Charge, Name, AtomicWeight, AtomicNumber FROM dbo.Elements WHERE Symbol = '" + symbol + "'";
    ret = SQLExecDirect(stmt, (SQLCHAR*)query.c_str(), SQL_NTS);
    
    if (!SQL_SUCCEEDED(ret)) {
        printf("Failed to execute query.\n");
        show_error(stmt, SQL_HANDLE_STMT);
        SQLFreeHandle(SQL_HANDLE_STMT, stmt);
        return;
    }

    // Get number of columns
    SQLSMALLINT columnCount;
    SQLNumResultCols(stmt, &columnCount);

    // Define buffer to hold data
    vector<SQLCHAR*> data(columnCount);
    vector<SQLLEN> dataLen(columnCount);
    
    // Allocate buffers
    for (int i = 0; i < columnCount; i++) {
        data[i] = new SQLCHAR[512];
    }

    // Fetch and display results
    if (SQL_SUCCEEDED(ret = SQLFetch(stmt))) {
        // Get column info
        SQLCHAR columnName[256];
        SQLSMALLINT columnNameLen;
        SQLSMALLINT dataType;
        SQLULEN columnSize;
        SQLSMALLINT decimalDigits;
        SQLSMALLINT nullable;
        
        // Get data into our buffers
        SQLLEN dataLen1, dataLen2, dataLen3, dataLen4, dataLen5;
        SQLCHAR symbolBuf[256] = {0};
        SQLCHAR chargeBuf[256] = {0};
        SQLCHAR nameBuf[256] = {0};
        SQLCHAR weightBuf[256] = {0};
        SQLCHAR numberBuf[256] = {0};
        
        // Get the data row directly
        SQLGetData(stmt, 1, SQL_C_CHAR, symbolBuf, sizeof(symbolBuf), &dataLen1);
        SQLGetData(stmt, 2, SQL_C_CHAR, chargeBuf, sizeof(chargeBuf), &dataLen2);
        SQLGetData(stmt, 3, SQL_C_CHAR, nameBuf, sizeof(nameBuf), &dataLen3);
        SQLGetData(stmt, 4, SQL_C_CHAR, weightBuf, sizeof(weightBuf), &dataLen4);
        SQLGetData(stmt, 5, SQL_C_CHAR, numberBuf, sizeof(numberBuf), &dataLen5);
        
        // Display data with printf to avoid encoding issues
        printf("\n--- Element Information ---\n");

        // Ensure we have the symbol to display
        // Check if we got the symbol from the database
        if (dataLen1 != SQL_NULL_DATA && symbolBuf[0] != '\0') {
            printf("Symbol:        %s\n", (char*)symbolBuf);
        } else {
            // Fall back to the user's input
            printf("Symbol:        %s\n", symbolDisplay.c_str());
        }
        
        printf("Name:          %s\n", dataLen3 == SQL_NULL_DATA ? "NULL" : (char*)nameBuf);
        printf("Atomic Number: %s\n", dataLen5 == SQL_NULL_DATA ? "NULL" : (char*)numberBuf);
        printf("Atomic Weight: %s g/mol\n", dataLen4 == SQL_NULL_DATA ? "NULL" : (char*)weightBuf);
        printf("Charge:        %s\n", dataLen2 == SQL_NULL_DATA ? "NULL" : (char*)chargeBuf);
    } else if (ret == SQL_NO_DATA) {
        printf("Element with symbol '%s' not found.\n", symbol.c_str());
    } else {
        printf("Error fetching data.\n");
        show_error(stmt, SQL_HANDLE_STMT);
    }

    // Clean up
    SQLFreeHandle(SQL_HANDLE_STMT, stmt);
}

// Function to get ion information (cation or anion)
bool getIonInfo(SQLHDBC dbc, const string& ionSymbol, bool isCation, double& charge, double& atomicMass) {
    SQLHSTMT stmt = SQL_NULL_HSTMT;
    SQLRETURN ret;
    
    // Default values in case of error
    charge = 0.0;
    atomicMass = 0.0;

    // Allocate statement handle
    ret = SQLAllocHandle(SQL_HANDLE_STMT, dbc, &stmt);
    if (!SQL_SUCCEEDED(ret)) {
        printf("Failed to allocate statement handle.\n");
        show_error(dbc, SQL_HANDLE_DBC);
        return false;
    }

    // Prepare SQL query with inline value - avoid parameter binding
    string tableName = isCation ? "dbo.Cations" : "dbo.Anions";
    string query = "SELECT Charge, AtomicWeight FROM " + tableName + " WHERE Symbol = '" + ionSymbol + "'";
    
    printf("Executing query: %s\n", query.c_str());

    ret = SQLExecDirect(stmt, (SQLCHAR*)query.c_str(), SQL_NTS);
    if (!SQL_SUCCEEDED(ret)) {
        printf("Failed to execute query.\n");
        show_error(stmt, SQL_HANDLE_STMT);
        SQLFreeHandle(SQL_HANDLE_STMT, stmt);
        return false;
    }

    // Use SQLGetData instead of binding
    if (SQL_SUCCEEDED(ret = SQLFetch(stmt))) {
        SQLCHAR chargeData[50] = {0};
        SQLCHAR weightData[50] = {0};
        SQLLEN lenCharge, lenWeight;
        
        // Get data for each column
        SQLGetData(stmt, 1, SQL_C_CHAR, chargeData, sizeof(chargeData), &lenCharge);
        SQLGetData(stmt, 2, SQL_C_CHAR, weightData, sizeof(weightData), &lenWeight);
        
        // Display the raw data
        printf("Raw charge data: '%s'\n", (lenCharge == SQL_NULL_DATA ? "NULL" : (char*)chargeData));
        printf("Raw weight data: '%s'\n", (lenWeight == SQL_NULL_DATA ? "NULL" : (char*)weightData));
        
        // Convert to needed types
        if (lenCharge != SQL_NULL_DATA) {
            try {
                charge = atof((char*)chargeData);
                printf("Retrieved charge: %.2f\n", charge);
            } catch (...) {
                printf("Warning: Could not convert charge value to a number\n");
                charge = isCation ? 1.0 : -1.0; // Default values
                printf("Using default charge: %.2f\n", charge);
            }
        } else {
            printf("Warning: Charge is NULL\n");
            charge = isCation ? 1.0 : -1.0; // Default values
            printf("Using default charge: %.2f\n", charge);
        }
        
        if (lenWeight != SQL_NULL_DATA) {
            try {
                atomicMass = atof((char*)weightData);
                printf("Retrieved atomic weight: %.4f\n", atomicMass);
            } catch (...) {
                printf("Warning: Could not convert atomic weight value to a number\n");
                atomicMass = 1.0; // Default value
                printf("Using default atomic weight: %.4f\n", atomicMass);
            }
        } else {
            printf("Warning: Atomic weight is NULL\n");
            atomicMass = 1.0; // Default value
            printf("Using default atomic weight: %.4f\n", atomicMass);
        }
        
        SQLFreeHandle(SQL_HANDLE_STMT, stmt);
        return true;
    } else if (ret == SQL_NO_DATA) {
        printf("%s with symbol '%s' not found.\n", (isCation ? "Cation" : "Anion"), ionSymbol.c_str());
    } else {
        printf("Error fetching data.\n");
        show_error(stmt, SQL_HANDLE_STMT);
    }

    SQLFreeHandle(SQL_HANDLE_STMT, stmt);
    return false;
}

// Function to list available ions
void listAvailableIons(SQLHDBC dbc, bool isCation) {
    SQLHSTMT stmt = SQL_NULL_HSTMT;
    SQLRETURN ret;
    
    // Allocate statement handle
    ret = SQLAllocHandle(SQL_HANDLE_STMT, dbc, &stmt);
    if (!SQL_SUCCEEDED(ret)) {
        printf("Failed to allocate statement handle.\n");
        show_error(dbc, SQL_HANDLE_DBC);
        return;
    }
    
    // Prepare SQL query
    string tableName = isCation ? "dbo.Cations" : "dbo.Anions";
    string query = "SELECT Symbol, Name FROM " + tableName + " ORDER BY Symbol";
    
    ret = SQLExecDirect(stmt, (SQLCHAR*)query.c_str(), SQL_NTS);
    if (!SQL_SUCCEEDED(ret)) {
        printf("Failed to execute query.\n");
        show_error(stmt, SQL_HANDLE_STMT);
        SQLFreeHandle(SQL_HANDLE_STMT, stmt);
        return;
    }
    
    // Variables to hold data
    SQLCHAR symbol[50] = {0};
    SQLCHAR name[100] = {0};
    SQLLEN lenSymbol, lenName;
    
    // Display header
    printf("\nAvailable %s:\n", (isCation ? "Cations" : "Anions"));
    printf("------------------------\n");
    printf("%-10s | %s\n", "Symbol", "Name");
    printf("------------------------\n");
    
    // Fetch and display results
    while (SQL_SUCCEEDED(ret = SQLFetch(stmt))) {
        // Get column data
        SQLGetData(stmt, 1, SQL_C_CHAR, symbol, sizeof(symbol), &lenSymbol);
        SQLGetData(stmt, 2, SQL_C_CHAR, name, sizeof(name), &lenName);
        
        // Print data
        printf("%-10s | %s\n", 
            (lenSymbol == SQL_NULL_DATA ? "NULL" : (char*)symbol),
            (lenName == SQL_NULL_DATA ? "NULL" : (char*)name));
    }
    
    printf("------------------------\n");
    
    // Clean up
    SQLFreeHandle(SQL_HANDLE_STMT, stmt);
}

// Function to calculate molar mass of ionic compound
void calculateMolarMass(SQLHDBC dbc) {
    string cationSymbol, anionSymbol;
    double cationCharge = 0.0, cationMass = 0.0, anionCharge = 0.0, anionMass = 0.0;
    
    printf("\n=== Ionic Compound Molar Mass Calculator ===\n");
    
    // List available cations
    listAvailableIons(dbc, true);
    
    // Get cation info
    printf("Enter cation symbol: ");
    cin >> cationSymbol;
    
    // Store original symbol for display
    string cationSymbolDisplay = cationSymbol;
    
    // Convert to uppercase for database lookup only
    for (char &c : cationSymbol) {
        c = toupper(c);
    }
    
    if (!getIonInfo(dbc, cationSymbol, true, cationCharge, cationMass)) {
        printf("Could not calculate molar mass due to missing cation information.\n");
        return;
    }
    
    // Verify cation charge is positive
    if (cationCharge <= 0) {
        printf("Error: Cation charge must be positive. The database shows %s with charge %.2f\n", 
               cationSymbol.c_str(), cationCharge);
        return;
    }
    
    // List available anions
    listAvailableIons(dbc, false);
    
    // Get anion info
    printf("Enter anion symbol: ");
    cin >> anionSymbol;
    
    // Store original symbol for display
    string anionSymbolDisplay = anionSymbol;
    
    // Convert to uppercase for database lookup only
    for (char &c : anionSymbol) {
        c = toupper(c);
    }
    
    if (!getIonInfo(dbc, anionSymbol, false, anionCharge, anionMass)) {
        printf("Could not calculate molar mass due to missing anion information.\n");
        return;
    }
    
    // Verify anion charge is negative
    if (anionCharge >= 0) {
        printf("Error: Anion charge must be negative. The database shows %s with charge %.2f\n",
               anionSymbol.c_str(), anionCharge);
        return;
    }
    
    // Following the logic from CalcMM function
    double totalMolarMass = 0.0;
    
    // Convert charges to their absolute values for calculation
    int cationChargeAbs = abs((int)cationCharge);
    int anionChargeAbs = abs((int)anionCharge);
    
    // Check if charges are equal
    if (cationChargeAbs == anionChargeAbs) {
        // Simple 1:1 compound
        totalMolarMass = cationMass + anionMass;
        
        // Display the result
        printf("\n--- Calculation Results ---\n");
        printf("Cation:        %s\n", cationSymbolDisplay.c_str());
        printf("Charge:        %.2f\n", cationCharge);
        printf("Atomic Weight: %.4f g/mol\n", cationMass);
        printf("\n");
        printf("Anion:         %s\n", anionSymbolDisplay.c_str());
        printf("Charge:        %.2f\n", anionCharge);
        printf("Atomic Weight: %.4f g/mol\n", anionMass);
        
        printf("\nThe charges have the same magnitude, so we have a 1:1 ratio.\n");
        printf("\nChemical formula: %s%s\n", cationSymbolDisplay.c_str(), anionSymbolDisplay.c_str());
        printf("Molar mass calculation: %.4f g/mol + %.4f g/mol = %.4f g/mol\n", 
               cationMass, anionMass, totalMolarMass);
    } else {
        // Need to cross-multiply charges for balancing
        int cationCount = anionChargeAbs;
        int anionCount = cationChargeAbs;
        
        // Simplify the ratio if possible (find GCD)
        int a = cationCount;
        int b = anionCount;
        while (b != 0) {
            int temp = b;
            b = a % b;
            a = temp;
        }
        int gcd = a;
        
        if (gcd > 1) {
            cationCount /= gcd;
            anionCount /= gcd;
        }
        
        // Calculate total molar mass according to your formula
        totalMolarMass = (anionChargeAbs * cationMass) + (cationChargeAbs * anionMass);
        
        // Display the result
        printf("\n--- Calculation Results ---\n");
        printf("Cation:        %s\n", cationSymbol.c_str());
        printf("Charge:        %.2f\n", cationCharge);
        printf("Atomic Weight: %.4f g/mol\n", cationMass);
        printf("\n");
        printf("Anion:         %s\n", anionSymbol.c_str());
        printf("Charge:        %.2f\n", anionCharge);
        printf("Atomic Weight: %.4f g/mol\n", anionMass);
        
        printf("\nCharge Balancing:\n");
        printf("To balance charges (%.2f and %.2f), we need:\n", cationCharge, anionCharge);
        printf("- %d %s ions (total charge: %.2f)\n", cationCount, cationSymbolDisplay.c_str(), cationCount * cationCharge);
        printf("- %d %s ions (total charge: %.2f)\n", anionCount, anionSymbolDisplay.c_str(), anionCount * anionCharge);
        printf("Total charge: %.2f (should be 0)\n", (cationCount * cationCharge) + (anionCount * anionCharge));
        
        printf("\nChemical formula: ");
        if (cationCount > 1) 
            printf("%s%d", cationSymbolDisplay.c_str(), cationCount);
        else 
            printf("%s", cationSymbolDisplay.c_str());
        
        if (anionCount > 1) 
            printf("%s%d", anionSymbolDisplay.c_str(), anionCount);
        else 
            printf("%s", anionSymbolDisplay.c_str());
        
        printf("\n");
        
        printf("Molar mass calculation: ");
        printf("(%.4f g/mol x %d) + ", cationMass, cationCount);
        printf("(%.4f g/mol x %d) = ", anionMass, anionCount);
        printf("%.4f g/mol\n", totalMolarMass);
    }
}

// Main program function
int main() {
    SQLHENV env = SQL_NULL_HENV;
    SQLHDBC dbc = SQL_NULL_HDBC;
    int choice;
    
    printf("=== Chemical Database Application ===\n");
    
    // Connect to database
    if (!connectToDatabase(env, dbc)) {
        printf("Press Enter to exit...");
        getchar();
        return 1;
    }
    
    // Main menu loop
    while (true) {
        printf("\n=== Main Menu ===\n");
        printf("1. Look up element information\n");
        printf("2. Calculate ionic compound molar mass\n");
        printf("3. Exit\n");
        printf("Enter your choice (1-3): ");
        
        // Validate input
        if (!(cin >> choice)) {
            cin.clear();
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            printf("Invalid input. Please enter a number.\n");
            continue;
        }
        
        // Process choice
        switch (choice) {
            case 1:
                getElementInfo(dbc);
                break;
            case 2:
                calculateMolarMass(dbc);
                break;
            case 3:
                printf("Disconnecting from database...\n");
                SQLDisconnect(dbc);
                SQLFreeHandle(SQL_HANDLE_DBC, dbc);
                SQLFreeHandle(SQL_HANDLE_ENV, env);
                printf("Thank you for using the Chemical Database Application. Goodbye!\n");
                printf("Press Enter to exit...");
                cin.ignore();
                getchar();
                return 0;
            default:
                printf("Invalid choice. Please enter a number between 1 and 3.\n");
        }
        
        // Clear input buffer
        cin.ignore(numeric_limits<streamsize>::max(), '\n');
    }
    
    return 0;
}
