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
#define MAX_FILE_COUNT 100

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
    int fileCount;   
    int fileIDs[MAX_FILE_COUNT];

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

typedef struct stackNode {
    struct graphNode* node; // Change commit* to graphNode*
    struct stackNode* next;
} stackNode;

typedef struct commitStack {
    struct stackNode* top;
} commitStack;

typedef struct queueNode {
    graphNode* node;
    struct queueNode* next;
} queueNode;

// Define a queue structure
typedef struct queue {
    queueNode* front;
    queueNode* rear;
} queue;



//-----------------FUNCTIONS--------------------------------------------
// Function to print the changes between two files
    graphNode* createGraphNode(commit* commit) {
        graphNode* newNode = (graphNode*)malloc(sizeof(graphNode));
        newNode->commit = commit;
        newNode->parent = NULL;
        newNode->nextParent = NULL;

        return newNode;
    }

    graphNode* createRepoNode(const char* repoName) {
        // Allocate memory for the new commit
        commit* newCommit = (commit*)malloc(sizeof(commit));
        if (newCommit == NULL) {
            printf("Error: Memory allocation failed.\n");
            return NULL;
        }

        // Populate commit fields
        snprintf(newCommit->message, sizeof(newCommit->message), "Initial commit for repository '%s'", repoName);
        newCommit->fileID = -1; // No file associated with the repo node
        snprintf(newCommit->author, sizeof(newCommit->author), "System");
        
        // Generating a timestamp (for example purposes)
        time_t t = time(NULL);
        struct tm* tm_info = localtime(&t);
        strftime(newCommit->timestamp, sizeof(newCommit->timestamp), "%Y-%m-%d %H:%M:%S", tm_info);

        // Create a graph node for the commit
        graphNode* newNode = createGraphNode(newCommit);

        return newNode;
    }

    repository* initRepository(const char* repoName) {
        repository* newRepo = (repository*)malloc(sizeof(repository));
        if (newRepo == NULL) {
            printf("Error: Memory allocation failed.\n");
            return NULL;
        }

        for (int i = 0; i < N; i++) {
            newRepo->nodes[i] = NULL;
            newRepo->fileHash[i] = NULL;
            newRepo->branches[i] = NULL;
        }
        newRepo->currentBranchIndex = -1; // No branch selected initially
        newRepo->branchCount = 0;

        // Set the default branch name to "main"
        const char* defaultBranchName = "main";
        newRepo->branches[0] = strdup(defaultBranchName);
        newRepo->branchCount++;

        // Create a graph node for the repository
        graphNode* repoNode = createRepoNode(repoName);
        newRepo->nodes[0] = repoNode; // Assuming index 0 for the repo node

        return newRepo;
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

    // Allocate memory for the new commit
    commit* newCommit = (commit*)malloc(sizeof(commit));
    if (newCommit == NULL) {
        printf("Error: Memory allocation failed.\n");
        fclose(file);
        return NULL;
    }

    // Populate commit fields
    snprintf(newCommit->message, sizeof(newCommit->message), "%s", message);
    newCommit->fileID = newFile->fileID;
    snprintf(newCommit->author, sizeof(newCommit->author), "%s", author);

    // Generating a timestamp (for example purposes)
    time_t t = time(NULL);
    struct tm* tm_info = localtime(&t);
    strftime(newCommit->timestamp, sizeof(newCommit->timestamp), "%Y-%m-%d %H:%M:%S", tm_info);

    // Close file
    fclose(file);

    // Create a graph node for the commit
    graphNode* newNode = createGraphNode(newCommit);

    // Add the new node to the repository's list of nodes
    index = newCommit->fileID % N;
    repo->nodes[index] = newNode;

    // Find the parent commit based on the current branch
    int currentBranchIndex = repo->currentBranchIndex;
    if (currentBranchIndex >= 0) {
        graphNode* parent = repo->nodes[currentBranchIndex]; // The parent is the head of the current branch
        if (parent != NULL) {
            // Add parent to the new node
            addParent(newNode, parent);
        }
    }

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

   void cloneBranch(repository* repo, const char* originalBranchName, const char* newBranchName) {
        // Find the index of the original branch
        int originalBranchIndex = -1;
        for (int i = 0; i < repo->branchCount; i++) {
            if (strcmp(repo->branches[i], originalBranchName) == 0) {
                originalBranchIndex = i;
                break;
            }
        }

        if (originalBranchIndex == -1) {
            printf("Error: Original branch '%s' not found.\n", originalBranchName);
            return;
        }

        // Clone the original branch
        repo->branches[repo->branchCount] = strdup(newBranchName);
        repo->branchCount++;

        // Set the current branch index to the new branch
        repo->currentBranchIndex = repo->branchCount - 1;

        printf("Branch '%s' created from branch '%s'.\n", newBranchName, originalBranchName);
    }
    void createBranch(repository* repo, const char* branchName) {
        if (repo->branchCount < N) {
            // Check if a branch with the same name already exists
            for (int i = 0; i < repo->branchCount; i++) {
                if (strcmp(repo->branches[i], branchName) == 0) {
                    printf("Error: Branch '%s' already exists.\n", branchName);
                    return;
                }
            }

            // Add the new branch to the repository
            repo->branches[repo->branchCount] = strdup(branchName);
            repo->branchCount++;

            printf("Branch created: %s\n", branchName);

            // If the newly created branch is not the main branch, clone the contents of the main branch into it
            if (strcmp(branchName, "main") != 0) {
                cloneBranch(repo, "main", branchName);
            }
        } else {
            printf("Error: Maximum number of branches reached.\n");
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

char* getFileContent(int fileID, repository* repo) {
    int hashIndex = fileID % N;
    File* currentFile = repo->fileHash[hashIndex];
    while (currentFile != NULL) {
        if (currentFile->fileID == fileID) {
            return currentFile->content;
        }
        currentFile = currentFile->next;
    }
    return "File not found";
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

repository* copyRepository(repository* originalRepo) {
    char repoName[100];
    printf("Enter repository name: ");
    scanf("%s", repoName);
    repository* newRepo = initRepository(repoName);

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
            int index = current->commit->fileID % N;
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
        File* currentFile = originalRepo->fileHash[i];
        while (currentFile != NULL) {
            // Create a new file
            File* newFile = (File*)malloc(sizeof(File));
            memcpy(newFile, currentFile, sizeof(File));

            // Add the new file to the new repository
            int index = currentFile->fileID % N;
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


commitStack* initCommitStack() {
    commitStack* stack = (commitStack*)malloc(sizeof(commitStack));
    stack->top = NULL;
    return stack;
}

void push(graphNode* node, commitStack* stack) { // Change commit* to graphNode*
    stackNode* newNode = (stackNode*)malloc(sizeof(stackNode));
    newNode->node = node; // Change commit to node
    newNode->next = stack->top;
    stack->top = newNode;
}

int isEmpty(commitStack* stack) {
    return stack->top == NULL;
}

graphNode* pop(commitStack* stack) { // Change commit* to graphNode*
    if (isEmpty(stack)) {
        printf("Error: Stack underflow\n");
        return NULL;
    }
    stackNode* temp = stack->top;
    graphNode* poppedNode = temp->node; // Change commit to node
    stack->top = temp->next;
    free(temp);
    return poppedNode; // Return commit pointer
}

queue* initQueue() {
    queue* q = (queue*)malloc(sizeof(queue));
    q->front = q->rear = NULL;
    return q;
}

// Function to check if the queue is empty
int isQueueEmpty(queue* q) {
    return q->front == NULL;
}

// Function to enqueue a node into the queue
void enqueue(graphNode* node, queue* q) {
    queueNode* newNode = (queueNode*)malloc(sizeof(queueNode));
    newNode->node = node;
    newNode->next = NULL;
    if (q->rear == NULL) {
        q->front = q->rear = newNode;
        return;
    }
    q->rear->next = newNode;
    q->rear = newNode;
}

// Function to dequeue a node from the queue
graphNode* dequeue(queue* q) {
    if (isQueueEmpty(q)) return NULL;
    queueNode* temp = q->front;
    graphNode* poppedNode = temp->node;
    q->front = q->front->next;
    if (q->front == NULL) q->rear = NULL;
    free(temp);
    return poppedNode;
}

void displayCommitInfo(graphNode* node) { // Change commit* to graphNode*
    printf("Commit ID: %d\n", node->commit->fileID);
    printf("Author: %s\n", node->commit->author);
    printf("Message: %s\n", node->commit->message);
    printf("\n");
}

void displayCommitHistory(commitStack* stack) {
    stackNode* current = stack->top;
    while (current != NULL) {
        displayCommitInfo(current->node);
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
    graphNode* recentCommit = pop(stack);

    // Find the graph node corresponding to the recent commit
    graphNode* recentNode = NULL;
    for (int i = 0; i < N; i++) {
        graphNode* current = repo->nodes[i];
        while (current != NULL) {
            if (current == recentCommit) {  // Compare the pointers directly
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

graphNode* undoMove(commitStack* stack) {
    if (isEmpty(stack)) {
        printf("Error: Commit history is empty.\n");
        return NULL;
    }
    // Get the pointer to the next graphNode without popping it
    return stack->top->next->node;  // Return the graphNode pointer
}

void pushCommitsUsingBFS(graphNode* startNode, commitStack* stack) {
    // Initialize a queue for BFS traversal
    queue* q = initQueue();
    
    // Array to keep track of visited nodes to avoid cycles
    int visited[N];
    memset(visited, 0, sizeof(visited));
    
    // Enqueue the start node and mark it as visited
    enqueue(startNode, q);
    visited[startNode->commit->fileID % N] = 1;

    // Perform BFS traversal
    while (!isQueueEmpty(q)) {
        graphNode* current = dequeue(q);
        push(current, stack); // Push the current node onto the stack

        // Enqueue unvisited parent nodes (children of the current node)
        graphNode* parent = current->parent;
        while (parent != NULL) {
            if (!visited[parent->commit->fileID % N]) {
                enqueue(parent, q);
                visited[parent->commit->fileID % N] = 1;
            }
            parent = parent->nextParent; // Corrected this line to traverse through nextParent
        }
    }

    // Free the queue memory
    free(q);
}

void printBranchContent(repository* repo, const char* branchName) {
    printf("Content of branch '%s':\n", branchName);

    // Find the index of the branch
    int branchIndex = -1;
    for (int i = 0; i < repo->branchCount; i++) {
        if (strcmp(repo->branches[i], branchName) == 0) {
            branchIndex = i;
            break;
        }
    }

    // Check if the branch exists
    if (branchIndex == -1) {
        printf("Error: Branch '%s' not found.\n", branchName);
        return;
    }

    // Get the head commit of the branch
    graphNode* headCommit = repo->nodes[branchIndex];

    // Print the content of each commit in the branch
    while (headCommit != NULL) {
        for (int i = 0; i < headCommit->commit->fileCount; i++) {
            int fileID = headCommit->commit->fileIDs[i];
            char* content = getFileContent(fileID, repo);
            printf("File ID: %d\n", fileID);
            printf("Content:\n%s\n", content);
            printf("------------------------\n");
        }
        headCommit = headCommit->nextParent; // Traverse through the nextParent to move to the next commit
    }
}


int main() {
    repository* myRepo ;
    commitStack* stack = initCommitStack();
    commitStack* stack2 = initCommitStack();
    graphNode* nextCommit;

    srand(time(NULL));
    char fileName[50];
    char updatedFileName[50];
    char message[100];
    char author[MAX_USERNAME_LENGTH];
    char branch[100];
    char branch2[100];
    char repoName[100];
    int choice;

    do {
        printf("\n--- Menu ---\n");
        printf("1. Init Repo\n");
        printf("2. Commit file\n");
        printf("3. Print file changes\n");
        printf("4. Create branch\n");
        printf("5. Checkout branch\n");
        printf("6. Merge\n");
        printf("7. Print repository\n");
        printf("8. Delete most recent commit\n");
        printf("9. Undo move\n");
        printf("10. Push commits using BFS\n");
        printf("11. Display commit history\n");
        printf("0. Exit\n");
        printf("Enter your choice: ");
        scanf("%d", &choice);

        switch (choice) {
            case 1:
                printf("Enter repository name: ");
                scanf("%s", repoName);

                repository* myRepo = initRepository(repoName);
                break;

            case 2:
                printf("Enter file name: ");
                scanf("%s", fileName);
                printf("Enter commit message: ");
                scanf("%s", message);
                printf("Enter author name: ");
                scanf("%s", author); 
                graphNode* commit  = commit_file(fileName, message, nextFileID, author, myRepo);
                push(commit, stack);
                break;
            case 3:
                printf("Enter original file name: ");
                scanf("%s", fileName);
                printf("Enter updated file name: ");
                scanf("%s", updatedFileName);
                printFileChanges(fileName, updatedFileName);
                break;
            case 4:
                printf("Enter the branch name: ");
                scanf("%s", branch);
                createBranch(myRepo, branch);
                break;

            case 5:
                printf("Enter the branch name: ");
                scanf("%s", branch);
                checkoutBranch(myRepo, branch);
                printBranchContent(myRepo, branch);
                break;
            case 6:
                printf("Enter commit 1 ID: ");
                int commitID1;
                scanf("%d", &commitID1);
                printf("Enter commit 2 ID: ");
                int commitID2;
                scanf("%d", &commitID2);
                graphNode* commit1 = myRepo->nodes[commitID1 % N];
                graphNode* commit2 = myRepo->nodes[commitID2 % N];
                merge(myRepo, commit1, commit2);
                break;
            case 7:
                printRepository(myRepo);
                break;
            case 8:
                deleteMostRecentCommit(myRepo, stack);
                break;
            case 9: 
                nextCommit = undoMove(stack);
                if (nextCommit != NULL) {
                    printf("Next commit after undo move:\n");
                    displayCommitInfo(nextCommit);
                    }

                break;
            case 10:
                printf("Enter commit ID: ");
                int bfsCommitID;
                scanf("%d", &bfsCommitID);
                graphNode* bfsCommit = myRepo->nodes[bfsCommitID % N];
                pushCommitsUsingBFS(bfsCommit, stack2);
                break;
            case 11:
                printf("History from stack:\n");
                displayCommitHistory(stack);
                printf("\nHistory from stack2:\n");
                displayCommitHistory(stack2);
                break;
            case 0:
                printf("Exiting program.\n");
                break;
            default:
                printf("Invalid choice. Please try again.\n");
        }
    } while (choice != 0);

    // Free allocated memory and exit
    // ...

    return 0;
}