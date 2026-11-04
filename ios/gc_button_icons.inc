// Original vector artwork, rasterized at the current display scale. No game assets.
static UIImage* gcButtonImage(uint16_t button){
    UIGraphicsImageRenderer* renderer=[[UIGraphicsImageRenderer alloc] initWithSize:CGSizeMake(64,40)];
    return [renderer imageWithActions:^(UIGraphicsImageRendererContext* context){
        UIColor* fill=[UIColor colorWithWhite:.7 alpha:1];CGRect box=CGRectMake(13,3,38,34);NSString* label=@"";
        if(button==PAD_A){fill=[UIColor colorWithRed:.12 green:.75 blue:.48 alpha:1];box=CGRectMake(14,2,36,36);label=@"A";}
        else if(button==PAD_B){fill=[UIColor colorWithRed:.9 green:.2 blue:.22 alpha:1];box=CGRectMake(20,8,24,24);label=@"B";}
        else if(button==PAD_X || button==PAD_Y){box=button==PAD_X?CGRectMake(8,10,48,20):CGRectMake(20,1,24,38);label=button==PAD_X?@"X":@"Y";}
        else if(button==PAD_Z){fill=[UIColor colorWithRed:.42 green:.32 blue:.8 alpha:1];box=CGRectMake(10,9,44,22);label=@"Z";}
        else if(button==PAD_L || button==PAD_R){box=CGRectMake(5,5,54,30);label=button==PAD_L?@"L":@"R";}
        else if(button==PAD_START){box=CGRectMake(17,5,30,30);label=@"START";}
        else {
            [fill setFill];UIBezierPath* cross=[UIBezierPath bezierPathWithRect:CGRectMake(24,2,16,36)];[cross appendPath:[UIBezierPath bezierPathWithRect:CGRectMake(14,12,36,16)]];[cross fill];
            [[UIColor colorWithWhite:.15 alpha:1] setFill];
            CGRect r=button==PAD_UP?CGRectMake(27,4,10,10):button==PAD_DOWN?CGRectMake(27,26,10,10):button==PAD_LEFT?CGRectMake(16,15,10,10):CGRectMake(38,15,10,10);
            [[UIBezierPath bezierPathWithRoundedRect:r cornerRadius:2] fill];return;
        }
        [fill setFill];[[UIBezierPath bezierPathWithRoundedRect:box cornerRadius:MIN(box.size.width,box.size.height)*.45] fill];
        NSDictionary* attrs=@{NSFontAttributeName:[UIFont boldSystemFontOfSize:button==PAD_START?7:18],NSForegroundColorAttributeName:[UIColor colorWithWhite:.12 alpha:1]};
        CGSize size=[label sizeWithAttributes:attrs];[label drawAtPoint:CGPointMake(32-size.width/2,20-size.height/2) withAttributes:attrs];
    }];
}
