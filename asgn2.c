#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>


#define MAX_LINE 1024
#define MAX_TOKENS 16
#define MAX_INODES 1024
#define NAME_LEN 32

int tok_line(char line[], char *tokens[], int max_tokens);
static void cmd_ls(uint32_t cwd, char inode_type[]);
static void cmd_cd(uint32_t *cwd, const char *name, char inode_type[]);
static void cmd_mkdir(uint32_t cwd, const char *name, char inode_type[], int inode_used[]);
static void cmd_touch(uint32_t cwd, const char *name, char inode_type[], int inode_used[]);
static void save_inodes_list(const char *path, int inode_used[], char inode_type[]);

int main(int argc, char *argv[]){

     if(argc != 2){
        fprintf(stderr, "Usage: fs_emulator <directory>\n");
        return 1;
    }

//PART2
    if(chdir(argv[1]) == -1){
        perror("chdir failed");
        return 1;
    }

    FILE *fp = fopen("inodes_list", "rb");
    if(fp == NULL){
        return 1;
    }

    char inode_type[MAX_INODES];
    int inode_used[MAX_INODES];


    for(int i = 0; i < MAX_INODES; i++){
        inode_type[i] = 0;
        inode_used[i] = 0;
    }
//test inodes are valid
    while(1){
    uint32_t ino;
    char type;

    if(fread(&ino, sizeof(ino), 1, fp) != 1){
        break;
    }
    if(fread(&type, sizeof(type), 1, fp) != 1){
        fprintf(stderr, "Invalid type\n");
        break;
    }

    if(ino >= MAX_INODES || (type != 'd' && type != 'f')){
        fprintf(stderr, "Invalid entry\n");
        continue;
    }

    inode_type[ino] = type;
    inode_used[ino] = 1;
    
    }
    fclose(fp);

//PART3
    uint32_t cwd = 0;
    if(inode_type[cwd] != 'd'){
        fprintf(stderr, "inode 0 not dir\n");
        return 1;
    }

//PART4
char line[MAX_LINE];
char *tokens[MAX_TOKENS];
while (1){
    printf("> ");
    fflush(stdout);

    if(fgets(line, sizeof(line), stdin) == NULL){
        save_inodes_list("inodes_list", inode_used, inode_type);
        break;
    }

    int ntok = tok_line(line, tokens, MAX_TOKENS);
    if(ntok == 0) continue;

    if(strcmp(tokens[0], "exit") == 0){
        save_inodes_list("inodes_list", inode_used, inode_type);
        exit(0);
    } else if (strcmp(tokens[0], "ls") == 0){
        if (ntok != 1){
            fprintf(stderr, "Usage: ls\n");
            continue;
        }
        cmd_ls(cwd, inode_type);
    } else if (strcmp(tokens[0], "cd") == 0){
        if (ntok != 2){
            fprintf(stderr, "Usage: cd <name>\n");
            continue;
        }
        cmd_cd(&cwd, tokens[1], inode_type);
    } else if (strcmp(tokens[0], "mkdir") == 0){
        if (ntok != 2){
            fprintf(stderr, "Usage: mkdir <name>\n");
            continue;
        }
        cmd_mkdir(cwd, tokens[1], inode_type, inode_used);
    } else if (strcmp(tokens[0], "touch") == 0){
        if (ntok != 2){
            fprintf(stderr, "Usage: touch <name>\n");
            continue;
        }
        cmd_touch(cwd, tokens[1], inode_type, inode_used);
    } else {
        fprintf(stderr, "Unknown command: %s\n", tokens[0]);
        continue;
    }
}

}


int tok_line(char line[], char *tokens[], int max_tokens){

    int i = 0;
    char *word = strtok(line, " \t\r\n\v\f");
        
    while(word != NULL && i < max_tokens - 1){
            tokens[i++] = word;
            word = strtok(NULL, " \t\r\n\v\f");
        }
    tokens[i] = NULL;
    return i;

}


static void cmd_ls(uint32_t cwd, char inode_type[]){
    char filename[32];
    snprintf(filename, sizeof(filename), "%u", cwd);

    FILE *f = fopen(filename, "rb");
    if(!f){
        perror("ls: open dir inode file");
        return;
    }

    while (1){
        uint32_t child;
        unsigned char name[NAME_LEN];

        if(fread(&child, sizeof(child), 1, f) != 1) break;
        if(fread(name, 1, NAME_LEN, f) != NAME_LEN){
            fprintf(stderr, "ls: dir file truncated\n");
            fclose(f);
            return;
        }
        if (child >= MAX_INODES) continue;
        printf("%u ", child);
        int len = 0;
        char real_name[NAME_LEN + 1];
        while(len < NAME_LEN && name[len] != '\0'){
            real_name[len] = name[len];
            len++;
        }
        real_name[len] = '\0';
        printf("%s\n", real_name);
    }
    fclose(f);
}


static void cmd_cd(uint32_t *cwd, const char *name, char inode_type[]){

    char filename[32];
    snprintf(filename, sizeof(filename), "%u", *cwd);

    FILE *f = fopen(filename, "rb");
    if(!f){
        perror("cd: open dir inode file");
        return;
    }

    while (1){
        uint32_t child;
        unsigned char raw[NAME_LEN];

        if(fread(&child, sizeof(child), 1, f) != 1) break;
        if(fread(raw, 1, NAME_LEN, f) != NAME_LEN){
            fprintf(stderr, "cd: dir file truncated\n");
            fclose(f);
            return;
        }

        int len = 0;
        char real_name[NAME_LEN + 1];
        while(len < NAME_LEN && raw[len] != '\0'){
            real_name[len] = raw[len];
            len++;
        }
        real_name[len] = '\0';

        if(strcmp(real_name, name) == 0){

            fclose(f);

            if(child >= MAX_INODES){
                fprintf(stderr, "cd: target inode %u not in use\n", child);
                return;
            }
            if(inode_type[child] != 'd'){
                fprintf(stderr, "cd: %s is not a directory\n", name);
                return;
            }

            *cwd = child;
            return;
        }

    }
    fclose(f);
    fprintf(stderr, "cd: no such directory: %s\n", name);
    return;
}

static void cmd_mkdir(uint32_t cwd, const char *name, char inode_type[], int inode_used[]){

    if (name == NULL || name[0] == '\0') {
        fprintf(stderr, "mkdir: invalid name\n");
        return;
    }

    char filename[32];
    snprintf(filename, sizeof(filename), "%u", cwd);

    FILE *f = fopen(filename, "rb");
    if(!f){
        perror("mkdir: change dir inode file");
        return;
    }

    while (1) {
        uint32_t child;
        unsigned char raw[NAME_LEN];

        if (fread(&child, sizeof(child), 1, f) != 1) break; // EOF
        if (fread(raw, 1, NAME_LEN, f) != NAME_LEN) {
            fclose(f);
            fprintf(stderr, "mkdir: parent directory file truncated/corrupt\n");
            return;
        }

        char entry[NAME_LEN + 1];
        int k = 0;
        while (k < NAME_LEN && raw[k] != '\0') {
            entry[k] = (char)raw[k];
            k++;
        }
        entry[k] = '\0';

        if (strcmp(entry, name) == 0) {
            fclose(f);
            fprintf(stderr, "mkdir: %s already exists\n", name);
            return;
        }
    }
    fclose(f);


    int found = -1;
    for (int i = 1; i < MAX_INODES; i++) {
        if (inode_used[i] == 0) {
            found = i;
            break;
        }
    }
    if(found == -1){
            fprintf(stderr, "mkdir: no available inodes\n");
            return;
        }
    
    uint32_t ndir = (uint32_t)found;
    inode_used[ndir] = 1;
    inode_type[ndir] = 'd';

    FILE *wf = fopen(filename, "ab"); 
    if (!wf) {
        perror("mkdir: open parent directory (append)");
        inode_used[ndir] = 0;
        inode_type[ndir] = 0;
        return;
    }

    {
        unsigned char rawname[NAME_LEN];
        memset(rawname, 0, NAME_LEN);
        size_t n = strlen(name);
        if (n > NAME_LEN) n = NAME_LEN;
        memcpy(rawname, name, n);

        if (fwrite(&ndir, sizeof(ndir), 1, wf) != 1 ||
            fwrite(rawname, 1, NAME_LEN, wf) != NAME_LEN) {
            fclose(wf);
            fprintf(stderr, "mkdir: failed writing parent directory entry\n");
            inode_used[ndir] = 0;
            inode_type[ndir] = 0;
            return;
        }
    }

    fclose(wf);

    char newfilename[32];
    snprintf(newfilename, sizeof(newfilename), "%u", ndir);
    FILE *fnew = fopen(newfilename, "wb");
    if(!fnew){
        perror("mkdir: failed to create inode dir file");
        inode_used[ndir] = 0;
        inode_type[ndir] = 0;
        return;
    }

    
    const char *dot = ".";
    unsigned char rawdot[NAME_LEN];
    memset(rawdot, 0, NAME_LEN);
    memcpy(rawdot, dot, 1);

    fwrite(&ndir, sizeof(ndir), 1, fnew);
    fwrite(rawdot, 1, NAME_LEN, fnew);

    const char *dotdot = "..";
    unsigned char rawdotdot[NAME_LEN];
    memset(rawdotdot, 0, NAME_LEN);
    memcpy(rawdotdot, dotdot, 2);

    fwrite(&cwd, sizeof(cwd), 1, fnew);
    fwrite(rawdotdot, 1, NAME_LEN, fnew);
    
    fclose(fnew);

}

static void cmd_touch(uint32_t cwd, const char *name, char inode_type[], int inode_used[]) {

    if (name == NULL || name[0] == '\0') {
        fprintf(stderr, "touch: invalid name\n");
        return;
    }
    char filename[32];
    snprintf(filename, sizeof(filename), "%u", cwd);

    FILE *f = fopen(filename, "rb");
    if (!f) {
        perror("touch: open cwd dir (read)");
        return;
    }

    while (1) {
        uint32_t child;
        unsigned char raw[NAME_LEN];

        if (fread(&child, sizeof(child), 1, f) != 1) break; 
        if (fread(raw, 1, NAME_LEN, f) != NAME_LEN) {
            fclose(f);
            fprintf(stderr, "touch: directory file truncated/corrupt\n");
            return;
        }

        char entry[NAME_LEN + 1];
        int k = 0;
        while (k < NAME_LEN && raw[k] != '\0') {
            entry[k] = (char)raw[k];
            k++;
        }
        entry[k] = '\0';

        if (strcmp(entry, name) == 0) {
            fclose(f);
            return; 
        }
    }
    fclose(f);

    int found = -1;
    for (int i = 1; i < MAX_INODES; i++) {
        if (inode_used[i] == 0) {
            found = i;
            break;
        }
    }
    if(found == -1){
            fprintf(stderr, "touch: no available inodes\n");
            return;
        }

    uint32_t nfile = (uint32_t)found;
    inode_used[nfile] = 1;
    inode_type[nfile] = 'f';

    FILE *wf = fopen(filename, "ab");
    if (!wf) {
        perror("touch: open cwd dir (append)");
        inode_used[nfile] = 0;
        inode_type[nfile] = 0;
        return;
    }
    unsigned char rawname[NAME_LEN];
    memset(rawname, 0, NAME_LEN);
    size_t n = strlen(name);
    if (n > NAME_LEN) n = NAME_LEN;
    memcpy(rawname, name, n);

    if (fwrite(&nfile, sizeof(nfile), 1, wf) != 1 || fwrite(rawname, 1, NAME_LEN, wf) != NAME_LEN) {
        fclose(wf);
        fprintf(stderr, "touch: failed writing directory entry\n");
        inode_used[nfile] = 0;
        inode_type[nfile] = 0;
        return;
    }
    fclose(wf);

      char newfilename[32];
    snprintf(newfilename, sizeof(newfilename), "%u", nfile);

    FILE *nf = fopen(newfilename, "wb");
    if (!nf) {
        perror("touch: create inode file");
        inode_used[nfile] = 0;
        inode_type[nfile] = 0;
        return;
    }
    fwrite(name, 1, strlen(name), nf);
    fwrite("\n", 1, 1, nf);
    fclose(nf);
}

static void save_inodes_list(const char *path, int inode_used[], char inode_type[]) {
    FILE *f = fopen(path, "wb");
    if (!f) {
        perror("exit: write inodes_list");
        return;
    }

    for (uint32_t i = 0; i < MAX_INODES; i++) {
        if (inode_used[i]) {
            char t = inode_type[i];
            if (t != 'd' && t != 'f') {
                // should never happen if your code is correct
                continue;
            }

            if (fwrite(&i, sizeof(i), 1, f) != 1 ||
                fwrite(&t, sizeof(t), 1, f) != 1) {
                perror("exit: fwrite inodes_list");
                fclose(f);
                return;
            }
        }
    }

    fclose(f);
}
