
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define MAX_FILE_CONTENT_SIZE 1000

int nextFileID = 1; // Global variable to track the next available file ID

typedef struct File {
    int fileID;
    char content[MAX_FILE_CONTENT_SIZE];
    struct File* next;
} File;

File* head = NULL; // Global variable to store the head of the linked list

// Function to print the changes between two files
void printFileChanges(const char* originalFileName, const char* updatedFileName) {
    FILE* originalFile = fopen(originalFileName, "r");
    FILE* updatedFile = fopen(updatedFileName, "r");

    if (originalFile == NULL || updatedFile == NULL) {
        printf("Error: Unable to open file.\n");
        return;
    }

    printf("Changes between %s and %s:\n", originalFileName, updatedFileName);

    char originalLine[MAX_FILE_CONTENT_SIZE];
    char updatedLine[MAX_FILE_CONTENT_SIZE];
    int lineNum = 1;
    int differencesFound = 0; // Flag to track if any differences were found

    // Compare lines of both files until one of them reaches the end
    while (fgets(originalLine, sizeof(originalLine), originalFile) != NULL &&
           fgets(updatedLine, sizeof(updatedLine), updatedFile) != NULL) {
        // Compare lines
        if (strcmp(originalLine, updatedLine) != 0) {
            printf("Difference found in line %d:\n", lineNum);
            printf("Original: %s", originalLine);
            printf("Updated: %s", updatedLine);
            printf("\n");
        }
        lineNum++;
    }

    // If one file has more lines than the other, print the remaining lines
    while (fgets(originalLine, sizeof(originalLine), originalFile) != NULL) {
        printf("Difference found in line %d:\n", lineNum);
        printf("Original: %s", originalLine);
        printf("Updated: (End of file)\n");
        printf("\n");
        differencesFound = 1;
        lineNum++;
    }

    while (fgets(updatedLine, sizeof(updatedLine), updatedFile) != NULL) {
        printf("Difference found in line %d:\n", lineNum);
        printf("Original: (End of file)\n");
        printf("Updated: %s", updatedLine);
        printf("\n");
        differencesFound = 1;
        lineNum++;
    }

    // If no differences were found, print a message
    if (!differencesFound) {
        printf("No differences found between %s and %s.\n", originalFileName, updatedFileName);
    }


    // Close files
    fclose(originalFile);
    fclose(updatedFile);
}

// Function to commit changes to a file
void commit(const char* fileName) {
    FILE* file = fopen(fileName, "r");
    if (file == NULL) {
        printf("Error: Unable to open file.\n");
        return;
    }

    // Allocate memory for the new file node
    File* newFile = (File*)malloc(sizeof(File));
    if (newFile == NULL) {
        printf("Error: Memory allocation failed.\n");
        fclose(file);
        return;
    }

    // Initialize file ID and content
    newFile->fileID = nextFileID++;
    newFile->next = NULL;
    newFile->content[0] = '\0'; // Initialize content to an empty string

    // Read file content line by line and concatenate to newFile->content
    char line[MAX_FILE_CONTENT_SIZE];
    while (fgets(line, sizeof(line), file) != NULL) {
        strcat(newFile->content, line);
    }

    // Close file
    fclose(file);

    // Store the new file in the linked list
    if (head == NULL) {
        head = newFile;
    } else {
        // Traverse to the end of the list
        File* temp = head;
        while (temp->next != NULL) {
            temp = temp->next;
        }
        // Append the new file
        temp->next = newFile;
    }

    printf("File committed successfully. File ID: %d\n", newFile->fileID);
}

// Function to print the contents of the linked list
void printFileList() {
    printf("File List:\n");
    File* temp = head;
    while (temp != NULL) {
        printf("File ID: %d\n", temp->fileID);
        printf("Content:\n%s\n", temp->content);
        temp = temp->next;
    }
}

int main() {
    char fileName[50];
    printf("Enter file name: ");
    scanf("%s", fileName);

    // Commit the original file
    commit(fileName);

    // Modify the file content (assuming the file is modified externally)
    // Here, you can write code to modify the file content

    // Assume the modified file is saved with a different name
    char updatedFileName[50];
    printf("Enter updated file name: ");
    scanf("%s", updatedFileName);

    // Print the changes between the original and updated files
    printFileChanges(fileName, updatedFileName);

    // Commit the updated file
    commit(updatedFileName);

    // Print the contents of the linked list
    printFileList();

    return 0;
}
