//
//  file.h
//  ice_sudoku
//
//  Created by 王子诚 on 2019/1/24.
//  Copyright © 2019 王子诚. All rights reserved.
//

#ifndef file_h
#define file_h

@interface filer: NSObject
{
    NSFileManager *FileManager;
};
-(NSString *)GetFilePath:(NSString*)filename;
-(void)File_Save:(NSString*)data to:(NSString*)filename;
-(NSString *)File_read:(NSString*)filename;
-(BOOL)Delete_File:(NSString*)filename;
-(void)File_Save_Mutable_Array:(NSMutableArray*)data to:(NSString*)filename;
-(NSMutableArray *)File_read_Mutable_Array:(NSString*)filename;
-(NSString *)pack_chessboard:(int[15][15])chessboard;
-(NSString *)pack_chess_stack:(int[225][2])stacker height:(int)stackheight;
-(void)release_chessboard:(int[15][15])chessboard data:(NSString*)data;
-(int)release_chess_stack:(int[225][2])stacker data:(NSString*)data;
@end
#endif /* file_h */
