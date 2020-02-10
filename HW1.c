#include <stdio.h> 
#include <string.h> 
#include <stdlib.h>

int GetTokenCount(char str[], char* delim);
char** GetTokens(char str[], char* delim, int tokenCount);

int main() 
{ 
    char str[] = "We hold these truths to be self-evident, that all men are created equal, that they are endowed, by their Creator, with certain unalienable Rights, that among these are Life, Liberty, and the pursuit of Happiness. That to secure these rights, Governments are instituted among Men, deriving their just powers from the consent of the governed, That whenever any Form of Government becomes destructive of these ends, it is the Right of the People to alter or abolish it, and to institute new Government, laying its foundation on such principles, and organizing its powers in such form, as to them shall seem most likely to effect their Safety and Happiness. Prudence, indeed, will dictate that Governments long established should not be changed for light and transient causes; and accordingly all experience hath shewn, that mankind are more disposed to suffer, while evils are sufferable, than to right themselves by abolishing the forms to which they are accustomed. But when a long train of abuses and usurpations, pursuing invariably the same Object, evinces a design to reduce them under absolute Despotism, it is their right, it is their duty, to throw off such Government, and to provide new Guards for their future security."; 
  

    int sentenceCount = GetTokenCount(strdup(str), ".");
    double average = 0;
  
    char** sentences = GetTokens(str, ".", sentenceCount);
    
    for (int i = 0; i < sentenceCount; i++)
    {
        average = average + GetTokenCount(sentences[i], " ");
    }

    average = average / sentenceCount;
    
    printf("There are %d sentences\n", sentenceCount);
    printf("Average words per sentence: %.2f\n", average);

  
    free(sentences);
    return 0; 
} 

int GetTokenCount(char str[], char* delim){
    int count = 0;
    
    char* token = strtok(str, delim); 

    while (token != NULL) { 
        count = count + 1; 
        token = strtok(NULL, delim); 
    } 

    return count;
}

char** GetTokens(char str[], char* delim, int tokenCount){
    char ** sub_str = malloc(tokenCount * sizeof(char*));
    
    char* token = strtok(str, delim); 


    for (int i = 0; i < tokenCount && token != NULL; i++)
    {
        sub_str[i] = strdup(token);
        token = strtok(NULL, delim); 
    }

    return sub_str;
} 