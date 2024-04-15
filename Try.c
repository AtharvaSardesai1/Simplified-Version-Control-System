#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <time.h>
#include <string.h>

#define N 10
#define MAX_FILE_CONTENT_SIZE 1000

int nextFileID = 1; // Global variable to track the next available file ID

typedef struct File {
    int fileID;
    char content[MAX_FILE_CONTENT_SIZE];
    struct File* next;
} File;

File* head = NULL; // Global variable to store the head of the linked list

typedef struct commit {
    char message[100];
    int fileID;
    char author[50];
    char timestamp[20];
} commit;

typedef struct graphNode {
    struct commit* commit;
    struct graphNode* parent; // Parent in the directed acyclic graph
    struct graphNode* nextParent; // Next parent in the linked list of parents
} graphNode;

typedef struct repository {
    struct graphNode* nodes[N];
    File* fileHash[N]; // Array of linked lists for file storage
    int currentBranchIndex; // Index of the current branch in the branch array
    int branchCount; // Total number of branches
    char* branches[N]; // Array to store branch names
} repository;



//-----------------FUNCTIONS--------------------------------------------
// Function to print the changes between two files
repository* initRepository() {
    repository* newRepo = (repository*)malloc(sizeof(repository));
    for (int i = 0; i < N; i++) {
        newRepo->nodes[i] = NULL;
        newRepo->fileHash[i] = NULL;
        newRepo->branches[i] = NULL;
    }
    newRepo->currentBranchIndex = -1; // No branch selected initially
    newRepo->branchCount = 0;
    
}


graphNode* createGraphNode(commit* commit) {
    graphNode* newNode = (graphNode*)malloc(sizeof(graphNode));
    newNode->commit = commit;
    newNode->parent = NULL;
    newNode->nextParent = NULL;

    return newNode;
}

void addParent(graphNode* child, graphNode* parent) {
    // Add parent to the beginning of the linked list
    parent->nextParent = child->parent;
    child->parent = parent;
}

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

graphNode* commit_file(const char* fileName, const char* message, int id, const char* author, repository* repo) {
    FILE* file = fopen(fileName, "r");
    if (file == NULL) {
        printf("Error: Unable to open file.\n");
        return NULL;
    }
    printf("22222222\n");

    // Allocate memory for the new file node
    File* newFile = (File*)malloc(sizeof(File));
    if (newFile == NULL) {
        printf("Error: Memory allocation failed.\n");
        fclose(file);
        return NULL;
    }

    // Initialize file ID and content
    newFile->fileID = id; // Assign the provided ID instead of incrementing nextFileID
    newFile->next = NULL;
    newFile->content[0] = '\0'; // Initialize content to an empty string

    printf("4444444\n");
    // Read file content line by line and concatenate to newFile->content
    char line[MAX_FILE_CONTENT_SIZE];
    size_t bytesRead;
    while ((bytesRead = fread(line, 1, sizeof(line), file)) > 0) {
        strncat(newFile->content, line, bytesRead);
    }

    // Store the new file in the linked list and file hash
    int index = id % N;
    if (repo->fileHash[index] == NULL) {
        repo->fileHash[index] = newFile;
    } else {
        newFile->next = repo->fileHash[index];
        repo->fileHash[index] = newFile;
    }

    printf("File added successfully. ID: %d\n", newFile->fileID);

    // Allocate memory for the new commit
    commit* newCommit = (commit*)malloc(sizeof(commit));
    if (newCommit == NULL) {
        printf("Error: Memory allocation failed.\n");
        fclose(file);
        return NULL;
    }
    printf("aaaaaaaaa\n");

    // Populate commit fields
    snprintf(newCommit->message, sizeof(newCommit->message), "%s", message);
    newCommit->fileID = newFile->fileID;
    snprintf(newCommit->author, sizeof(newCommit->author), "%s", author);
    printf("bbbbbbbbbb\n");

    // Generating a timestamp (for example purposes)
    time_t t = time(NULL);
    struct tm* tm_info = localtime(&t);
    strftime(newCommit->timestamp, sizeof(newCommit->timestamp), "%Y-%m-%d %H:%M:%S", tm_info);
    printf("cccccccccc\n");

    // Close file
    fclose(file);

    // Create a graph node for the commit
    graphNode* newNode = createGraphNode(newCommit);

    return newNode;
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

void createBranch(repository* repo, const char* branchName) {
    if (repo->branchCount < N) {
        repo->branches[repo->branchCount] = strdup(branchName);
        repo->branchCount++;
        printf("Branch created: %s\n", branchName);
    } else {
        printf("Maximum number of branches reached.\n");
    }
}

// Function to switch to a different branch
void checkoutBranch(repository* repo, const char* branchName) {
    for (int i = 0; i < repo->branchCount; i++) {
        if (strcmp(repo->branches[i], branchName) == 0) {
            repo->currentBranchIndex = i;
            printf("Switched to branch: %s\n", branchName);
            return;
        }
    }
    printf("Branch not found: %s\n", branchName);
}

void freeCommitTree(graphNode* root) {
    if (root != NULL) {
        freeCommitTree(root->nextParent);
        free(root->commit);
        free(root);
    }
}

void printRepository(repository* repo) {
    printf("Repository Contents:\n");
    printf("Current Branch Index: %d\n", repo->currentBranchIndex);
    printf("Branch Count: %d\n", repo->branchCount);
    
    // Print branches
    printf("Branches:\n");
    for (int i = 0; i < repo->branchCount; i++) {
        printf("%d: %s\n", i, repo->branches[i]);
    }

    // Print file hash
    printf("File Hash:\n");
    for (int i = 0; i < N; i++) {
        printf("Index %d:\n", i);
        File* file = repo->fileHash[i];
        while (file != NULL) {
            printf("File ID: %d\n", file->fileID);
            printf("Content:\n%s\n", file->content);
            file = file->next;
        }
    }

    // Print commit nodes
    printf("Commit Nodes:\n");
    for (int i = 0; i < N; i++) {
        printf("Index %d:\n", i);
        graphNode* node = repo->nodes[i];
        while (node != NULL) {
            printf("Message: %s\n", node->commit->message);
            printf("File ID: %d\n", node->commit->fileID);
            printf("Author: %s\n", node->commit->author);
            printf("Timestamp: %s\n", node->commit->timestamp);
            node = node->nextParent;
        }
    }
}


int main() {
    repository* myRepo = initRepository();

    srand(time(NULL));
    char fileName[50];
    printf("Enter file name: ");
    scanf("%s", fileName);

    char message[100];
    printf("Enter commit message: ");
    scanf("%s", message); // Assuming single-word messages for simplicity

    // Assuming you have some way to get the author name, for example:
    const char* author = "John Doe"; 
 

    // Commit the original file with the provided message and author
    commit_file(fileName, message, nextFileID, author, myRepo);
    printf("ggggggggg\n");

    // Modify the file content (assuming the file is modified externally)
    // Here, you can write code to modify the file content

    // Assume the modified file is saved with a different name
    char updatedFileName[50];
    printf("Enter updated file name: ");
    scanf("%s", updatedFileName);

    printf("Enter commit message: ");
    scanf("%s", message); // Assuming single-word messages for simplicity

    // Print the changes between the original and updated files
    printFileChanges(fileName, updatedFileName);

    // Commit the updated file
    commit_file(updatedFileName, message, nextFileID, author, myRepo);

    char branch[100];
    printf("Enter the branch name: ");
    scanf("%s", branch);

    char branch2[100];
    printf("Enter the branch name: ");
    scanf("%s", branch2);

    createBranch(myRepo, branch);
    createBranch(myRepo, branch2);

    printRepository(myRepo);


    checkoutBranch(myRepo, branch);

    checkoutBranch(myRepo, branch2);




    // Print the contents of the linked list
    //printFileList();

    return 0;
}