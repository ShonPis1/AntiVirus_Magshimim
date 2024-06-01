#include <stdio.h>
#include "dirent.h"
#include <stdbool.h>


#define AMOUNT_ARGS 3
#define FAILURE_CODE -1

enum SCANNING_OPTIONS {
	NORMAL,
	FAST
};

bool argumentsChecker(int amout, char** args);
bool directoryExists(char* path);
bool fileExists(char* path);
int menu(char* folderPath, char* signaturePath, FILE* logFile);
int countFiles(DIR* directory);
char** getFileNames(DIR* directory, int filesCount, FILE* logFile);
void sortFileNames(char** arr, int size);
int calcFileSize(FILE* file);
char* getBFileContent(FILE* file, FILE* logFile);
char* scanFile(char* signatureContent, FILE* file, int option, FILE* logFile);
bool findSignature(FILE* file, char* signatureContent, char* fileContent, float fromPrec, float toPrec);
void printAndLog(char* msg, FILE* logFile);

int main(int argc, char** argv)
{
	if (!argumentsChecker(argc, argv))
	{
		printf("Usage: %s <directory> <virus_signature_file>\n", argv[0]);
		return FAILURE_CODE;
	}
	const char* directoryPath = argv[1];
	const char* virusSignaturePath = argv[2];
	
	//create the log path
	char logPath[PATH_MAX];
	strcpy(logPath, "");
	strcat(logPath, directoryPath);
	strcat(logPath, "/");
	strcat(logPath, "myAntiVirusLog.txt");
	FILE* logFile = fopen(logPath, "w+");
	if (!logFile)
	{
		printf("error to create the log file! ");
		return FAILURE_CODE;
	}

	int choice = menu(directoryPath, virusSignaturePath, logFile); //getting the selected scan option

	//getting file signature
	FILE* signatureFile = fopen(virusSignaturePath, "rb");
	char* signatureContent = getBFileContent(signatureFile, logFile);
	fclose(signatureFile);

	// getting the file's names
	DIR* directory = opendir(directoryPath);
	int filesAmount = countFiles(directory);
	char** fileNames = getFileNames(directory, filesAmount, logFile);
	closedir(directory);
	sortFileNames(fileNames, filesAmount);

	char filePath[PATH_MAX]; //PATH_MAX is a define in dirent.h for the buffer of the path string 
	for (int i = 0; i < filesAmount; i++) 
	{
		// crearting path each file  
		strcpy(filePath, ""); 
		strcat(filePath, directoryPath);
		strcat(filePath, "/");
		strcat(filePath, fileNames[i]);

		FILE* file = fopen(filePath, "rb");
		if (!file)
		{
			printf("Failed to open the file: %s\n", filePath);
			fprintf(logFile, "Failed to open the file: %s\n", filePath);
			return FAILURE_CODE;
		}

		char* fileState = scanFile(signatureContent, file, choice, logFile);
		fclose(file);

		if (fileState != NULL)
		{
			printf("%s Infected!%s\n", filePath, fileState);
			fprintf(logFile, "%s Infected!%s\n", filePath, fileState);
		}
		else
		{
			printf("%s Clean!\n", filePath);
			fprintf(logFile, "%s Clean!\n", filePath);
		}
	}
	printf("\nSee the results on %s", logPath);
	fclose(logFile);
	free(signatureContent);
	
	for (int i = 0; i < filesAmount; i++)
	{
		free(fileNames[i]);
	}
	free(fileNames);
	return 0;
}

/*checks if the entered arguments to the main are valid
input: the amount of arguments and the arguments
output: if the args are valid or not*/
bool argumentsChecker(int amout, char** args)
{
	if (amout != AMOUNT_ARGS)
		return FALSE;	

	if (!directoryExists(args[1])) 
		return FALSE;
	if (!fileExists(args[2]))
		return FALSE;
	
	return TRUE;
}

/*checks if the path is a directory and exists
* input: the path of the directory
* output: if it dir or not
*/
bool directoryExists(char* path)
{
	DIR* checkedDir = opendir(path);
	if (checkedDir) 
	{
		//The directory exists
		closedir(checkedDir);	
		return TRUE;
	}
	// Not a directory,or may be a file or does not exist
	printf("dir doesnt exist\n");
	return FALSE;
}

/*checks if the path is is file and exists
input: the path of the file
output: if the file is exists or not */
bool fileExists(char* path) 
{
	FILE* checkedFile = fopen(path, "r");
	if (checkedFile)
	{
		// File exists
		fclose(checkedFile);
		return TRUE;
	}
	// File does not exist
	printf("file doesnt exist\n");
	return FALSE;
}

/*prints the menu and gets the user scan option and for the log file
input: the directory's and file's path and the log file
output: the option normal or fast*/
int menu(char* folderPath, char* signaturePath, FILE* logFile)
{
	char choice;
	printf("Welcome to my Virus Scan! \n\n");
	fprintf(logFile, "Welcome Anti-Virus began!\n\n");

	printf("Folder to scan: %s\n", folderPath);
	fprintf(logFile, "Folder to scan: %s\n", folderPath);

	printf("Virus signature: %s\n\n", signaturePath);
	fprintf(logFile, "Virus signature: %s\n\n", signaturePath);

	printf("Press 0 for a normal scan or any other key for a quick scan: ");

	scanf("%c", &choice);
	fprintf(logFile, "Scanning option:\n");
	if(choice == '0')
		fprintf(logFile, "Normal Scan\n\n");
	else
		fprintf(logFile, "Quick Scan\n\n");

	printf("Scanning began...\n");
	printf("This process may take several minutes...\n\n");
	printf("Scanning:\n");
	fprintf(logFile, "Results:\n");
	if (choice == '0')
	{
		return NORMAL;
	}
	return FAST;
}

/*counts the amount of files in the entered dir
input: the directory
output: the amount*/
int countFiles(DIR* directory)
{
	int fileCounter = 0;
	struct dirent* dir;

	while ((dir = readdir(directory)) != NULL) //enter into the dir 
	{
		if (dir->d_type == DT_REG) //checking if it's a regular file 
			fileCounter++;
	}

	rewinddir(directory);
	return fileCounter;
}

/*gets all the file names in the entered dir
input: the dir and amount of files in and the log file
output: array of all the paths*/
char** getFileNames(DIR* directory, int filesCount, FILE* logFile)
{
	struct dirent* dir;
	char** fileNames;

	fileNames = (char**)malloc(sizeof(char*) * filesCount); //creating an array for storing the file's paths
	if (!fileNames)
	{
		printf("allocation failed\n");
		fprintf(logFile, "allocation failed\n");
		exit(FAILURE_CODE); //ending the program 
	}

	for (int i = 0; (dir = readdir(directory)) != NULL;) 
	{
		if (dir->d_type == DT_REG) //checking if the file is regular
		{
			fileNames[i] = (char*)malloc(sizeof(char) * (dir->d_namlen + 1)); //added 1 for the null
			if (!(fileNames[i]))
			{
				printf("allocation failed\n");
				fprintf(logFile, "allocation failed\n");
				exit(FAILURE_CODE); //ending the program 
			}

			strcpy(fileNames[i], dir->d_name); 
			i++;
		}
	}	
	rewinddir(directory);
	return fileNames;
}

/*sorts the array of file names by using bubble sort
input: the array and it's size
output: none*/
void sortFileNames(char** arr, int size)
{
	char temp[PATH_MAX];
	for (int i = 0; i < size - 1; i++)
	{
		for (int j = 0; j < size - i - 1; j++) 
		{
			if (strcmp(arr[j], arr[j + 1]) > 0)
			{
				strcpy(temp, arr[j]);
				strcpy(arr[j], arr[j + 1]);
				strcpy(arr[j + 1], temp); 
			}
		}
	}
}

/*calcs the size in bytes of file
input: the file
output: the size*/
int calcFileSize(FILE* file)
{
	fseek(file, 0, SEEK_END);
	int fileSize = ftell(file);
	rewind(file);
	return fileSize;
}

/*scans the content of binary file into a string 
input: the path of the file and log file
output: the content*/
char* getBFileContent(FILE* file, FILE* logFile)
{
	int fileSize = calcFileSize(file);

	char* fileContent = (char*)malloc(sizeof(char) * (fileSize + 1));
	if (!fileContent)
	{
		printf("allocation is failed!!\n");
		fprintf(logFile, "allocation is failed!!\n");
		exit(FAILURE_CODE); //ending the program 
	}

	fread(fileContent, 1, fileSize, file);
	fileContent[fileSize] = '\0'; 

	return fileContent;
}

/*reads the content of a file and scans for a signature within the file content
and returns a string indicating where the signature was found based on the selected search option
or if clean null 
input: the content of the signature, the file, the user option
output: where the signature was found  based on the selected search option or if clean null*/
char* scanFile(char* signatureContent, FILE* file, int option, FILE* logFile) 
{
	char* fileContent = getBFileContent(file, logFile);
	char* answer = NULL;

	switch (option)
	{
		case NORMAL:
			answer = findSignature(file, signatureContent, fileContent, 0, 1) ? "" : NULL;
		case FAST:
		{
			if (findSignature(file, signatureContent, fileContent, 0, 0.2))
				answer = " (first 20%)";
			if (findSignature(file, signatureContent, fileContent, 0.8, 1))
				answer = " (last 20%)";
			if (findSignature(file, signatureContent, fileContent, 0.2, 0.8))
				answer = "";
		}
	}

	free(fileContent);
	return answer;
}


/*searches the signature of the virus in the content of the checked file
input: the file, the content of the virus signature, the content of the file, 
initial percentage of the file, final percentage of the file

output: if the signature is exist of not*/
bool findSignature(FILE* file, char* signatureContent, char* fileContent, float fromPerc, float toPerc)
{
	int fileSize = calcFileSize(file);

	//index is the place where the checking is starting from in the file content
	for (int index = (int)(fileSize * fromPerc); index < (int)(fileSize * toPerc);)
	{
		// if the strstr finds null it refer to it as the end of the string
		//checking parts of the string contents until it finds NULL
		if (strstr((fileContent + index), signatureContent) != NULL)
			return TRUE; //the file infected 
		index += strlen(fileContent + index) + 1; //pass the NULL char and try checking again
		//back to the loop if there is more string in the range after passing the null or stopping the loop
	}
	return FALSE; //file is clear 
}

void printAndLog(char* msg, FILE* logFile)
{
	printf(msg);
	fprintf(logFile, msg);
}