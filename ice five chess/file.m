//
//  file.m
//  ice_sudoku
//
//  Created by 王子诚 on 2019/1/24.
//  Copyright © 2019 王子诚. All rights reserved.
//

#import <Foundation/Foundation.h>
#include "file.h"

@implementation filer
-(NSString *)GetFilePath:(NSString*)filename
{
    NSArray* paths = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES);
    NSString* documentsDirectory = [paths objectAtIndex:0];
    return [documentsDirectory stringByAppendingPathComponent:filename];
}
-(void)File_Save:(NSString*)data to:(NSString*)filename
{
    NSError * error;
    [data writeToFile:[self GetFilePath:filename] atomically:YES encoding:NSUTF8StringEncoding error:&error];
       // NSLog(@"save %@ to %@",data,filename);
}
-(NSString *)File_read:(NSString*)filename
{
    NSError * error;
    NSString* res=[[NSString alloc]initWithContentsOfFile:[self GetFilePath:filename] encoding:NSUTF8StringEncoding error:&error];
    //NSLog(@"read %@ from %@",res,filename);
    return res;
}

-(NSString *)pack_chessboard:(int[15][15])chessboard;
{
    char dir[226]={};
    for(int i=0;i<15;i++)
        for(int j=0;j<15;j++)
            dir[i*15+j]=(char)(chessboard[i][j]+48);
    dir[225]='\0';
    return [[NSString alloc] initWithCString:dir encoding:NSUTF8StringEncoding];
}
-(NSString *)pack_chess_stack:(int[225][2])stacker height:(int)stackheight;
{
    char dir[451]={};
    for(int i=0;i<stackheight;i++)
    {
        for(int j=0;j<2;j++)
            dir[i*2+j]=(char)(stacker[i][j]+65);
    }
    dir[stackheight*2]='\0';
    return [[NSString alloc] initWithCString:dir encoding:NSUTF8StringEncoding];
}
-(void)release_chessboard:(int[15][15])chessboard data:(NSString*)data;
{
    if(data.length<220)
        return;
    const char* dir=[data UTF8String];
    int le=0;
    for(int i=0;i<15;i++)
        for(int j=0;j<15;j++)
        {
            chessboard[i][j]=(int)(dir[le]-'0');
            ++le;
        }
}
-(int)release_chess_stack:(int[225][2])stacker data:(NSString*)data
{
    int stackheight=(int)data.length/2;
    const char* dir=[data UTF8String];
    for(int i=0;i<stackheight;i++)
    {
        for(int j=0;j<2;j++)
          stacker[i][j]=(int)(dir[i*2+j]-65);
    }
    return stackheight;
}
-(BOOL)Delete_File:(NSString*)filename
{
    if(FileManager==nil)
        FileManager=[[NSFileManager alloc]init];
    
    if([FileManager removeItemAtPath:[self GetFilePath:filename] error:NULL]==NO)
    {
        // NSLog(@"Delete %@ failed",filename);
        return NO;
    }
    
    return true;
}
-(void)File_Save_Mutable_Array:(NSMutableArray*)data to:(NSString*)filename
{
    [data writeToFile:[self GetFilePath:filename] atomically:true];
}
-(NSMutableArray*)File_read_Mutable_Array:(NSString*)filename
{
    NSMutableArray*data=[[NSMutableArray alloc]initWithContentsOfFile:[self GetFilePath:filename]];
    return data;
}
@end
