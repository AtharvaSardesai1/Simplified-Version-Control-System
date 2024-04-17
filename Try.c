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
        struct commit* commit;
        struct stackNode* next;
    } stackNode;

    typedef struct commitStack {
        struct stackNode* top;
    } commitStack;

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
        if (repo->nodes[index] == NULL) {
            repo->nodes[index] = newNode;
        } else {
            // Find the last node in the list
            graphNode* temp = repo->nodes[index];
            while (temp->nextParent != NULL) {
                temp = temp->nextParent;
            }
            // Add the new node to the end of the list
            temp->nextParent = newNode;
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

    //----------------STACK OPERATIONS----------------
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

    void displayCommitInfo(commit* commit) {
        printf("Commit ID: %d\n", commit->fileID);
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

    void dfs(graphNode* node, int* visited) {
    if (node == NULL || visited[node->commit->fileID]) {
        return;
    }
    visited[node->commit->fileID] = 1;
    
    // Print information about the commit node
    printf("Message: %s\n", node->commit->message);
    printf("File ID: %d\n", node->commit->fileID);
    printf("Author: %s\n", node->commit->author);
    printf("Timestamp: %s\n", node->commit->timestamp);
    printf("\n");

    // Traverse all parents of the current node
    graphNode* parent = node->parent;
    while (parent != NULL) {
        dfs(parent, visited);
        parent = parent->nextParent;
    }
}


void printRepository(repository* repo) {
    printf("Repository Contents:\n");
    printf("Current Branch Index: %d\n", repo->currentBranchIndex);
    printf("Branch Count: %d\n", repo->branchCount);

    printf("Branches:\n");
    for (int i = 0; i < repo->branchCount; i++) {
        printf("%d: %s\n", i, repo->branches[i]);
    }

    printf("File Hash:\n");
    // Print file hash contents (omitted for brevity)

    printf("Commit Nodes:\n");
    for (int i = 0; i < N; i++) {
        printf("Index %d:\n", i);
        graphNode* node = repo->nodes[i];
        while (node != NULL) {
            printf("Message: %s\n", node->commit->message);
            printf("File ID: %d\n", node->commit->fileID);
            printf("Author: %s\n", node->commit->author);
            printf("Timestamp: %s\n", node->commit->timestamp);
            printf("\n");
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

    // Function to merge two text files with conflict resolution
    void mergeFiles(repository* repo, int fileID1, int fileID2) {
        // Retrieve content of the text files from their respective branches
        char* content1 = getFileContent(fileID1, repo);
        char* content2 = getFileContent(fileID2, repo);

        // Compare content to identify differences
        if (strcmp(content1, content2) == 0) {
            // No conflicts, files are identical
            printf("No conflicts detected. Files are identical.\n");
            return;
        }

        // Detect changes in individual lines
        char* line1 = strtok(content1, "\n");
        char* line2 = strtok(content2, "\n");
        int lineNum = 1;
        int conflicts = 0; // Flag to track if conflicts were detected
        while (line1 != NULL || line2 != NULL) {
            if (line1 != NULL && line2 != NULL) {
                if (strcmp(line1, line2) != 0) {
                    // Conflict detected
                    printf("Conflict detected in line %d.\n", lineNum);
                    printf("Content from file 1: %s\n", line1);
                    printf("Content from file 2: %s\n", line2);
                    // Resolve conflict manually or automatically
                    // For simplicity, let's choose content from file 2
                    printf("Choosing changes from file 2.\n");
                    conflicts++;
                }
            } else if (line1 != NULL) {
                // Additional lines in file 1
                printf("Additional lines found in file 1 (line %d):\n", lineNum);
                printf("%s\n", line1);
                conflicts++;
            } else if (line2 != NULL) {
                // Additional lines in file 2
                printf("Additional lines found in file 2 (line %d):\n", lineNum);
                printf("%s\n", line2);
                conflicts++;
            }
            line1 = strtok(NULL, "\n");
            line2 = strtok(NULL, "\n");
            lineNum++;
        }

        if (conflicts == 0) {
            // No conflicts, merge successful
            printf("Merge successful. Merged content:\n");
            // Choose content from file 2 and update file 1 with it
            int hashIndex = fileID1 % N;
            File* currentFile = repo->fileHash[hashIndex];
            while (currentFile != NULL) {
                if (currentFile->fileID == fileID1) {
                    // Replace content with content from file 2
                    strncpy(currentFile->content, content2, MAX_FILE_CONTENT_SIZE);
                    printf("%s\n", content2); // Print merged content
                    break;
                }
                currentFile = currentFile->next;
            }
        } else {
            // Conflicts detected, merge aborted
            printf("Merge aborted due to conflicts.\n");
        }
    }


    int main() {

        char repoName[100];
        printf("Enter repository name: ");
        scanf("%s", repoName);

        repository* myRepo = initRepository(repoName);


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
        graphNode* commit  = commit_file(fileName, message, nextFileID, author, myRepo);
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
        graphNode* commit2 = commit_file(updatedFileName, message, nextFileID, author, myRepo);

        char branch[100];
        printf("Enter the branch name: ");
        scanf("%s", branch);

        cloneBranch(myRepo, "main", branch);
        printRepository(myRepo);

        checkoutBranch(myRepo, branch);

        // Display the contents of the new branch after checkout
        printf("Contents of branch '%s' after checkout:\n", branch);
        printRepository(myRepo);

        //merge(myRepo, commit, commit2);
        // Print the contents of the linked list
        //printFileList();

        return 0;
    }
