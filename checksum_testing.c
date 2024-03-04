#include<stdio.h>
#include<math.h>

void *getOldSettings(char *buf[]) {
   
  buf[0] = "hi";
  buf[1] = "bye";
  
}

void *getCurrentSettings(char *buf[]) {
   
  buf[0] = "hi";
  buf[1] = "hello";
  
}
 
int sender(char *sender_checksum)
{
    int checksum = 0;
    int sum=0;
    
    printf("\n****SENDER SIDE****\n");
    
    for(int i = 0; i < 2; i++) {
        sum += sender_checksum[i];
    }
    
    printf("SUM IS: %d",sum);
    
    checksum =~ sum;    //1's complement of sum
    
    printf("\nCHECKSUM IS:%d",checksum);
    
    return checksum;
}
 
int receiver(char *receiver_checksum, int sender_checksum)
{
    int checksum = 0;
    int sum=0;
    
    printf("\n\n****RECEIVER SIDE****\n");
    
    for(int i = 0;i < 2; i++) {
        sum+=receiver_checksum[i];
    
    }
    
    printf("SUM IS:%d",sum);
    
    sum=sum+sender_checksum;
    
    checksum=~sum;    //1's complement of sum
    
    printf("\nCHECKSUM IS:%d",checksum);
    
    return checksum;
}
 
void main()
{
    int sender_checksum,receiver_checksum;
    char *oldSettings[2], *currentSettings[2];

    getOldSettings(oldSettings);
    
    for(int i = 0; i < 2; i++) {
        printf(oldSettings[i]);
        printf("\n");
    }
    
    getCurrentSettings(currentSettings);
    
    for(int i = 0; i < 2; i++) {
        printf(currentSettings[i]);
        printf("\n");
    }

    sender_checksum = sender(*oldSettings);
    receiver_checksum = receiver(*currentSettings, sender_checksum);
}