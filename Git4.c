#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <time.h>
#include <string.h>

#define N 10
#define MAX_FILES_PER_COMMIT 5
#define MAX_FILE_CONTENT_SIZE 40000
#define MAX_USERS 100
#define MAX_USERNAME_LENGTH 50
#define MAX_PASSWORD_LENGTH 50


// STRUCTURES

typedef struct file {
    int id; // Unique file ID
    char name[50];
    char content[MAX_FILE_CONTENT_SIZE];
    struct file* next; // Linked list pointer
} file;

typedef struct commit {
    char message[100];
    int id;
    char author[50];
    char timestamp[20];
    int fileCount;
    int fileIDs[MAX_FILES_PER_COMMIT];
} commit;

typedef struct graphNode {
    struct commit* commit;
    struct graphNode* parent; // Parent in the directed acyclic graph
    struct graphNode* nextParent; // Next parent in the linked list of parents
} graphNode;

typedef struct repository {
    struct graphNode* nodes[N];
    struct file* fileHash[N]; // Array of linked lists for file storage
    int currentBranchIndex; // Index of the current branch in the branch array
    int branchCount; // Total number of branches
    char* branches[N]; // Array to store branch names
} repository;

typedef struct stackNode {
    struct commit* commit;
    struct stackNode* next;
} stackNode;

typedef struct commitStack {
    struct stackNode* top;
} commitStack;

// Structure to represent a user
typedef struct {
    char username[MAX_USERNAME_LENGTH];
    char password[MAX_PASSWORD_LENGTH];
} User;

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

// FUNCTIONS

commit* createCommit(const char* message, int id, const char* author) {
    commit* newCommit = (commit*)malloc(sizeof(commit));
    snprintf(newCommit->message, sizeof(newCommit->message), "%s", message);
    newCommit->id = id;
    snprintf(newCommit->author, sizeof(newCommit->author), "%s", author);

    // Generating a timestamp (for example purposes)
    time_t t = time(NULL);
    struct tm* tm_info = localtime(&t);
    strftime(newCommit->timestamp, sizeof(newCommit->timestamp), "%Y-%m-%d %H:%M:%S", tm_info);

    newCommit->fileCount = 0; // Initialize file count to zero

    return newCommit;
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

void addFileFunction(repository* repo) {
    char fileName[MAX_USERNAME_LENGTH];
    char filePath[MAX_USERNAME_LENGTH];
    int choice;
    
    printf("Select input type:\n");
    printf("1. Text input\n");
    printf("2. File input\n");
    printf("Enter your choice: ");
    scanf("%d", &choice);
    getchar(); // Consume the newline character left in the input buffer
    
    if (choice == 1) {
        // Text input
        printf("Enter text name: ");
        fgets(fileName, MAX_USERNAME_LENGTH, stdin);
        fileName[strcspn(fileName, "\n")] = '\0';  // Remove newline character
        
        printf("Enter text content: ");
        fgets(filePath, MAX_FILE_CONTENT_SIZE, stdin);
        filePath[strcspn(filePath, "\n")] = '\0';  // Remove newline character
    } else if (choice == 2) {
        // File input
        printf("Enter file name: ");
        fgets(fileName, MAX_USERNAME_LENGTH, stdin);
        fileName[strcspn(fileName, "\n")] = '\0';  // Remove newline character
        
        printf("Enter file path: ");
        fgets(filePath, MAX_USERNAME_LENGTH, stdin);
        filePath[strcspn(filePath, "\n")] = '\0';  // Remove newline character
    } else {
        printf("Invalid choice.\n");
        return;
    }
    
    // Create a new file node
    file* newFile = (file*)malloc(sizeof(file));
    newFile->id = rand(); // Assign a unique file ID
    snprintf(newFile->name, sizeof(newFile->name), "%s", fileName);
    
    if (choice == 1) {
        // Copy text content to file content
        snprintf(newFile->content, sizeof(newFile->content), "%s", filePath);
        printf("File added successfully. ID: %d\n", newFile->id);
    } else if (choice == 2) {
        // Open the file
        FILE* filePtr = fopen(filePath, "r");
        if (filePtr != NULL) {
            // Read file content
            size_t bytesRead = fread(newFile->content, 1, MAX_FILE_CONTENT_SIZE - 1, filePtr);
            if (bytesRead > 0) {
                newFile->content[bytesRead] = '\0'; // Null-terminate the content
                printf("File added successfully. ID: %d\n", newFile->id);
                
                // Store the file in the repository's file hash
                int index = newFile->id % N;
                if (repo->fileHash[index] == NULL) {
                    repo->fileHash[index] = newFile;
                } else {
                    newFile->next = repo->fileHash[index];
                    repo->fileHash[index] = newFile;
                }
            } else {
                perror("Error reading file content");
            }
            fclose(filePtr);
        } else {
            perror("Error opening file");
        }
    }
}


void addFileToCommit(commit* commit, const char* name, const char* filePath, repository* repo) {
    if (commit->fileCount < MAX_FILES_PER_COMMIT) {
        int fileID = rand(); // Use a better method for real projects

        commit->fileIDs[commit->fileCount] = fileID;

        FILE* file = fopen(filePath, "rb");
        if (file != NULL) {
            size_t bytesRead = fread(repo->fileHash[fileID % N]->content, 1, MAX_FILE_CONTENT_SIZE - 1, file);
            if (bytesRead > 0) {
                repo->fileHash[fileID % N]->content[bytesRead] = '\0'; // Null-terminate the content
            } else {
                perror("Error reading file content");
            }
            fclose(file);
        } else {
            perror("Error opening file");
            return;
        }

        commit->fileCount++;
        printf("File added successfully. ID: %d\n", fileID);
    } else {
        printf("Maximum number of files per commit reached.\n");
    }
}

void displayCommit(commit* commit, repository* repo) {
    printf("Commit ID: %d\n", commit->id);
    printf("Author: %s\n", commit->author);
    printf("Message: %s\n", commit->message);
    printf("Timestamp: %s\n", commit->timestamp);
    printf("Files:\n");
    for (int i = 0; i < commit->fileCount; i++) {
        int fileID = commit->fileIDs[i];
        printf("  Name: file%d.txt\n", fileID);

        int hashIndex = fileID % N;
        file* currentFile = repo->fileHash[hashIndex];
        while (currentFile != NULL) {
            if (currentFile->id == fileID) {
                printf("  Content:\n%s\n", currentFile->content);
                break;
            }
            currentFile = currentFile->next;
        }
    }
    printf("\n");
}

void displayDataFunction(repository* repo) {
    int fileId;
    printf("Enter file ID: ");
    scanf("%d", &fileId);
    
    // Find the file in the repository's file hash
    int index = fileId % N;
    file* currentFile = repo->fileHash[index];
    while (currentFile != NULL) {
        if (currentFile->id == fileId) {
            // Create a dummy commit with the file and call displayCommit function
            commit* dummyCommit = createCommit("Dummy commit", 0, "Dummy author");
            dummyCommit->fileCount = 1;
            dummyCommit->fileIDs[0] = fileId;
            
            displayCommit(dummyCommit, repo);
            
            // Free memory associated with the dummy commit
            free(dummyCommit);
            return;
        }
        currentFile = currentFile->next;
    }
    
    printf("File not found.\n");
}

char* getFileContent(int fileID, repository* repo) {
    int hashIndex = fileID % N;
    file* currentFile = repo->fileHash[hashIndex];
    while (currentFile != NULL) {
        if (currentFile->id == fileID) {
            return currentFile->content;
        }
        currentFile = currentFile->next;
    }
    return "File not found";
}

void freeCommitTree(graphNode* root) {
    if (root != NULL) {
        freeCommitTree(root->nextParent);
        free(root->commit);
        free(root);
    }
}

// Stack functions

commitStack* initCommitStack() {
    commitStack* stack = (commitStack*)malloc(sizeof(commitStack));
    stack->top = NULL;
    return stack;
}

void push(commit* commit, commitStack* stack) {
    stackNode* newNode = (stackNode*)malloc(sizeof(stackNode));
    newNode->commit = commit;
    newNode->next = stack->top;
    stack->top = newNode;
}

int isEmpty(commitStack* stack) {
    return stack->top == NULL;
}

commit* pop(commitStack* stack) {
    if (isEmpty(stack)) {
        printf("Error: Stack underflow\n");
        return NULL;
    }
    stackNode* temp = stack->top;
    commit* poppedCommit = temp->commit;
    stack->top = temp->next;
    free(temp);
    return poppedCommit;
}

// Function to create a new branch
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

void displayCommitInfo(commit* commit) {
    printf("Commit ID: %d\n", commit->id);
    printf("Author: %s\n", commit->author);
    printf("Message: %s\n", commit->message);
    printf("\n");
}

void displayCommitHistory(commitStack* stack) {
    stackNode* current = stack->top;
    while (current != NULL) {
        displayCommitInfo(current->commit);
        current = current->next;
    }
}

void deleteMostRecentCommit(repository* repo, commitStack* stack) {
    // Check if the stack is empty
    if (isEmpty(stack)) {
        printf("Error: Commit history is empty.\n");
        return;
    }

    // Pop the most recent commit from the stack
    commit* recentCommit = pop(stack);

    // Find the graph node corresponding to the recent commit
    graphNode* recentNode = NULL;
    for (int i = 0; i < N; i++) {
        graphNode* current = repo->nodes[i];
        while (current != NULL) {
            if (current->commit == recentCommit) {
                recentNode = current;
                break;
            }
            current = current->nextParent;
        }
        if (recentNode != NULL)
            break;
    }

    // If the recent node is not found, something went wrong
    if (recentNode == NULL) {
        printf("Error: Recent commit node not found.\n");
        return;
    }

    // Remove the recent node from the DAG
    for (int i = 0; i < N; i++) {
        graphNode* current = repo->nodes[i];
        graphNode* prev = NULL;
        while (current != NULL) {
            if (current == recentNode) {
                if (prev != NULL) {
                    prev->nextParent = current->nextParent;
                } else {
                    repo->nodes[i] = current->nextParent;
                }
                free(current->commit); // Free memory associated with the commit
                free(current); // Free memory associated with the graph node
                return;
            }
            prev = current;
            current = current->nextParent;
        }
    }
}

commit* undoMove(commitStack* stack) {
    if (isEmpty(stack)) {
        printf("Error: Commit history is empty.\n");
        return NULL;
    }
    // Get the pointer to the next commit without popping it
    return stack->top->next->commit;
}

repository* copyRepository(repository* originalRepo) {
    repository* newRepo = initRepository();

    // Copy commits and graph nodes
    for (int i = 0; i < N; i++) {
        graphNode* current = originalRepo->nodes[i];
        while (current != NULL) {
            // Create a new commit
            commit* newCommit = (commit*)malloc(sizeof(commit));
            memcpy(newCommit, current->commit, sizeof(commit));

            // Create a new graph node
            graphNode* newNode = createGraphNode(newCommit);

            // Add the new graph node to the new repository
            int index = current->commit->id % N;
            if (newRepo->nodes[index] == NULL) {
                newRepo->nodes[index] = newNode;
            } else {
                newNode->nextParent = newRepo->nodes[index];
                newRepo->nodes[index] = newNode;
            }

            current = current->nextParent;
        }
    }

    // Copy files
    for (int i = 0; i < N; i++) {
        file* currentFile = originalRepo->fileHash[i];
        while (currentFile != NULL) {
            // Create a new file
            file* newFile = (file*)malloc(sizeof(file));
            memcpy(newFile, currentFile, sizeof(file));

            // Add the new file to the new repository
            int index = currentFile->id % N;
            if (newRepo->fileHash[index] == NULL) {
                newRepo->fileHash[index] = newFile;
            } else {
                newFile->next = newRepo->fileHash[index];
                newRepo->fileHash[index] = newFile;
            }

            currentFile = currentFile->next;
        }
    }

    return newRepo;
}

graphNode* findCommonAncestor(graphNode* commit1, graphNode* commit2) {
    // Traverse ancestors of commit1 and check if they are ancestors of commit2
    graphNode* ancestor1 = commit1;
    while (ancestor1 != NULL) {
        graphNode* ancestor2 = commit2;
        while (ancestor2 != NULL) {
            if (ancestor1 == ancestor2) {
                return ancestor1; // Common ancestor found
            }
            ancestor2 = ancestor2->parent;
        }
        ancestor1 = ancestor1->parent;
    }

    return NULL; // No common ancestor found
}

void applyChanges(repository* repo, graphNode* commit, graphNode* commonAncestor) {
    // Apply changes from commit to the current branch
    while (commit != commonAncestor) {
        // Iterate over files in the commit and merge changes
        for (int i = 0; i < commit->commit->fileCount; i++) {
            int fileID = commit->commit->fileIDs[i];
            char* content = getFileContent(fileID, repo);

            // Check if file exists in common ancestor
            char* ancestorContent = getFileContent(fileID, repo);

            if (strcmp(content, ancestorContent) != 0) {
                // Files have diverged, handle conflict resolution here
                printf("Conflict detected in file %d. Manual resolution required.\n", fileID);
            } else {
                // Files are the same, no conflict
                // Perform automatic merge (if applicable) or keep the content as-is
                printf("File %d merged successfully.\n", fileID);
            }
        }

        // Move to the parent commit
        commit = commit->parent;
    }
}


void merge(repository* repo, graphNode* commit1, graphNode* commit2) {
    // Find common ancestor(s) of commit1 and commit2
    graphNode* commonAncestor = findCommonAncestor(commit1, commit2);

    if (commonAncestor == NULL) {
        printf("No common ancestor found. Merge aborted.\n");
        return;
    }

    // Apply changes from commit1 and commit2 to the current branch
    applyChanges(repo, commit1, commonAncestor);
    applyChanges(repo, commit2, commonAncestor);

    printf("Merge successful.\n");
}



void gitLog(repository* repo, graphNode* commit) {
    while (commit != NULL) {
        displayCommit(commit->commit, repo);
        commit = commit->parent; // Move to the parent commit
    }
}

// Function to register a new user
void signup(User* users, int* userCount) {
    if (*userCount >= MAX_USERS) {
        printf("Maximum number of users reached.\n");
        return;
    }

    printf("Enter username: ");
    fgets(users[*userCount].username, MAX_USERNAME_LENGTH, stdin);
    users[*userCount].username[strcspn(users[*userCount].username, "\n")] = '\0';  // Remove newline character

    printf("Enter password: ");
    fgets(users[*userCount].password, MAX_PASSWORD_LENGTH, stdin);
    users[*userCount].password[strcspn(users[*userCount].password, "\n")] = '\0';  // Remove newline character

    (*userCount)++;
    printf("User registered successfully.\n");
}

// Function to authenticate a user
int login(User* users, int userCount) {
    char username[MAX_USERNAME_LENGTH];
    char password[MAX_PASSWORD_LENGTH];

    printf("Enter username: ");
    fgets(username, MAX_USERNAME_LENGTH, stdin);
    username[strcspn(username, "\n")] = '\0';  // Remove newline character

    printf("Enter password: ");
    fgets(password, MAX_PASSWORD_LENGTH, stdin);
    password[strcspn(password, "\n")] = '\0';  // Remove newline character

    for (int i = 0; i < userCount; i++) {
        if (strcmp(users[i].username, username) == 0 && strcmp(users[i].password, password) == 0) {
            return i;  // Return index of the logged-in user
        }
    }

    return -1;  // Login failed
}

// MAIN FUNCTION


int main() {
    
    repository* myRepo ;
    User users[MAX_USERS];
    int userCount = 0;
    int loggedInUserIndex = -1;

    int choice;
    char input[100];

    while (1) {
        printf("\n");
        printf("1. Login\n");
        printf("2. Signup\n");
        printf("0. Exit\n");
        printf("Enter your choice: ");
        scanf("%d", &choice);
        getchar(); // Clear input buffer

        switch (choice) {
            case 1: // Login
                loggedInUserIndex = login(users, userCount);
                if (loggedInUserIndex != -1) {
                    printf("Login successful.\n");
                } else {
                    printf("Invalid username or password.\n");
                }
                break;

            case 2: // Signup
                signup(users, &userCount);
                break;

            case 0: // Exit
                printf("Exiting program.\n");
                // Free memory and exit
                // Remember to free memory allocated for users and repository
                return 0;

            default:
                printf("Invalid choice. Please enter a valid option.\n");
                break;
        }


        char repoName[MAX_USERNAME_LENGTH];
        printf("Enter repository name: ");
        fgets(repoName, MAX_USERNAME_LENGTH, stdin);
        repoName[strcspn(repoName, "\n")] = '\0';  // Remove newline character
        printf("Repository initialized with name: %s\n", repoName);
        return myRepo;

        if (loggedInUserIndex != -1) {
            // Start menu
            while (1) {
                printf("\n");
                printf("1. Init\n");
                printf("2. Add file\n");
                printf("3. Display data\n");
                printf("4. History\n");
                printf("5. Branch\n");
                printf("6. Merge\n");
                printf("7. Git log\n");
                printf("8. Logout\n");
                printf("Enter your choice: ");
                scanf("%d", &choice);
                getchar(); // Clear input buffer

                switch (choice) {
                    case 1: // Init
                        initRepository(myRepo);
                        break;
                            
                    case 2: // Add file
                        addFileFunction(myRepo);
                        break;

                    case 3: // Display data
                        printf("Displaying data:\n");
                        displayDataFunction(myRepo);
                        break;

                    case 4: // History
                        // Implement history functionality
                        // historyFunction();
                        break;

                    case 5: // Branch
                        // Implement branching functionality
                        // branchFunction();
                        break;

                    case 6: // Merge
                        // Implement merging functionality
                        // mergeFunction();
                        break;

                    case 7: // Git log
                        // Implement git log functionality
                        // gitLogFunction();
                        break;

                    case 8: // Logout
                        loggedInUserIndex = -1; // Logout
                        printf("Logged out successfully.\n");
                        break;

                    default:
                        printf("Invalid choice. Please enter a valid option.\n");
                        break;
                }

                if (choice == 8) {
                    break; // Exit the inner while loop if user chooses to logout
                }
            }
        }
    }

    return 0;
}
