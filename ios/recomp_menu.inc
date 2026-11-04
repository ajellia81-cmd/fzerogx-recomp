// UIKit methods included inside GameController. Input remains live for this menu
// while guest execution, GPU submission, rumble and audio are parked.
- (void)syncPause {
    bool pause=self.menuOpen || !self.appActive;
    if(pause){
        if(self.appActive)ios_rumble_pause_menu();else ios_rumble_suspend();
        ios_lifecycle::gate().pause();
        if(self.started)irq_request();
        // Backgrounding must drain before UIKit suspends us. An in-app menu
        // keeps the surface alive and can appear while the renderer parks.
        if(!self.appActive){
            ios_lifecycle::gate().wait_for_renderer();
            if(self.started)ios_audio_suspend();
        }
        [self.touchControls resetInput];
        PadState neutral{};neutral.connected=ios_controller_snapshot().connected;pad_set_state(0,neutral);
    }else if(ios_lifecycle::gate().resume()){
        ++fpsEpoch;if(self.started)ios_audio_resume();
        ios_rumble_resume();
    }
}
- (void)foregroundChanged:(BOOL)active {
    self.appActive=active;
    self.inputLink.paused=!active;
    self.previousButtons=ios_controller_snapshot().buttons;
    [self syncPause];
}
- (void)toggleRecomp {
    if(!self.started)return;
    self.menuOpen=!self.menuOpen;self.menuIndex=0;self.menuPage=0;self.bindingTarget=-1;
    [self syncPause];
    if(self.menuOpen)[self showRecompPage];else{[self.recompPanel removeFromSuperview];self.recompPanel=nil;++fpsEpoch;}
}
- (void)showRecompPage {
    [self.recompPanel removeFromSuperview];
    self.recompPanel=[[UIView alloc] initWithFrame:self.view.bounds];
    self.recompPanel.autoresizingMask=UIViewAutoresizingFlexibleWidth|UIViewAutoresizingFlexibleHeight;
    self.recompPanel.backgroundColor=[UIColor colorWithWhite:0 alpha:.85];
    [self.view addSubview:self.recompPanel];
    self.recompStack=[[UIStackView alloc] init];
    self.recompStack.axis=UILayoutConstraintAxisVertical;self.recompStack.spacing=4;
    self.recompStack.translatesAutoresizingMaskIntoConstraints=NO;
    self.recompTitle=[[UILabel alloc] init];self.recompTitle.textColor=UIColor.whiteColor;
    self.recompTitle.textAlignment=NSTextAlignmentCenter;self.recompTitle.numberOfLines=2;
    self.recompTitle.text=self.menuPage==1?@"버튼 지정":self.menuPage==2?@"지정할 패드 버튼을 눌러 주세요":@"Recomp 메뉴";
    [self.recompStack addArrangedSubview:self.recompTitle];
    NSArray<NSString*>* titles=@[[NSString stringWithFormat:@"Smooth motion : %@",self.smoothEnabled?@"켜짐":@"꺼짐"],[NSString stringWithFormat:@"FPS 표시 : %@",self.fpsEnabled?@"켜짐":@"꺼짐"],@"버튼 지정",@"게임으로 돌아가기"];
    NSArray<NSString*>* physical=@[@"A",@"B",@"X",@"Y",@"LB",@"RB",@"START",@"↑",@"↓",@"←",@"→",@"LT",@"RT"];
    if(self.menuPage==1){NSMutableArray* t=[NSMutableArray array];for(auto target:ios_bindings::targets){NSMutableArray* sources=[NSMutableArray array];for(unsigned j=0;j<13;++j)if(ios_controller_binding(j)==target)[sources addObject:physical[j]];[t addObject:sources.count?[sources componentsJoinedByString:@" / "]:@"미지정"];}[t addObject:@"기본 배치 복원"];[t addObject:@"돌아가기"];titles=t;}
    if(self.menuPage==2)titles=@[@"지정 해제",@"취소"];
    NSMutableArray* buttons=[NSMutableArray array];
    for(unsigned i=0;i<titles.count;++i){UIButton* b=[UIButton buttonWithType:UIButtonTypeSystem];[b setTitle:titles[i] forState:UIControlStateNormal];
        if(self.menuPage==1 && i<ios_bindings::targets.size()){[b setImage:[gcButtonImage(ios_bindings::targets[i]) imageWithRenderingMode:UIImageRenderingModeAlwaysOriginal] forState:UIControlStateNormal];b.imageView.contentMode=UIViewContentModeScaleAspectFit;NSArray* labels=@[@"게임큐브 A",@"게임큐브 B",@"게임큐브 X",@"게임큐브 Y",@"게임큐브 Z",@"게임큐브 L",@"게임큐브 R",@"게임큐브 START",@"십자키 위",@"십자키 아래",@"십자키 왼쪽",@"십자키 오른쪽"];b.accessibilityLabel=[NSString stringWithFormat:@"%@, %@",labels[i],titles[i]];}
        b.tag=i;b.titleLabel.font=[UIFont systemFontOfSize:16];[b.heightAnchor constraintEqualToConstant:44].active=YES;[b addTarget:self action:@selector(menuButton:) forControlEvents:UIControlEventTouchUpInside];[self.recompStack addArrangedSubview:b];[buttons addObject:b];}
    self.menuButtons=buttons;self.menuIndex=MIN(self.menuIndex,(NSInteger)buttons.count-1);
    self.recompScroll=[[UIScrollView alloc] init];self.recompScroll.translatesAutoresizingMaskIntoConstraints=NO;[self.recompPanel addSubview:self.recompScroll];[self.recompScroll addSubview:self.recompStack];
    [NSLayoutConstraint activateConstraints:@[[self.recompScroll.centerXAnchor constraintEqualToAnchor:self.recompPanel.centerXAnchor],[self.recompScroll.topAnchor constraintEqualToAnchor:self.recompPanel.safeAreaLayoutGuide.topAnchor constant:12],[self.recompScroll.bottomAnchor constraintEqualToAnchor:self.recompPanel.safeAreaLayoutGuide.bottomAnchor constant:-12],[self.recompScroll.widthAnchor constraintEqualToAnchor:self.recompPanel.safeAreaLayoutGuide.widthAnchor multiplier:.75],[self.recompStack.topAnchor constraintEqualToAnchor:self.recompScroll.contentLayoutGuide.topAnchor],[self.recompStack.bottomAnchor constraintEqualToAnchor:self.recompScroll.contentLayoutGuide.bottomAnchor],[self.recompStack.leadingAnchor constraintEqualToAnchor:self.recompScroll.contentLayoutGuide.leadingAnchor],[self.recompStack.trailingAnchor constraintEqualToAnchor:self.recompScroll.contentLayoutGuide.trailingAnchor],[self.recompStack.widthAnchor constraintEqualToAnchor:self.recompScroll.frameLayoutGuide.widthAnchor]]];
    [self highlightMenu];
}
- (void)highlightMenu {if(self.menuButtons.count){[self.recompPanel layoutIfNeeded];[self.recompScroll scrollRectToVisible:self.menuButtons[self.menuIndex].frame animated:NO];}for(unsigned i=0;i<self.menuButtons.count;++i)self.menuButtons[i].backgroundColor=(NSInteger)i==self.menuIndex?[UIColor colorWithRed:.12 green:.25 blue:.4 alpha:1]:UIColor.clearColor;}
- (void)menuButton:(UIButton*)button {[self activateMenuRow:button.tag];}
- (void)activateMenuRow:(NSInteger)row {
    if(self.menuPage==1){if(row<12){self.bindingTarget=row;self.menuPage=2;}else if(row==12)ios_controller_reset_bindings();else self.menuPage=0;self.menuIndex=0;[self showRecompPage];return;}
    if(self.menuPage==2){if(row==0)ios_controller_clear(ios_bindings::targets[self.bindingTarget]);self.menuPage=1;self.menuIndex=self.bindingTarget;[self showRecompPage];return;}
    if(row==2){self.menuPage=1;self.menuIndex=0;[self showRecompPage];return;}
    if(row==0){
        if(UIScreen.mainScreen.maximumFramesPerSecond>=120)self.smoothEnabled=!self.smoothEnabled;
        smoothMotion=self.smoothEnabled;[NSUserDefaults.standardUserDefaults setBool:self.smoothEnabled forKey:@"recomp.smooth"];
        self.inputLink.preferredFrameRateRange=self.smoothEnabled?CAFrameRateRangeMake(120,120,120):CAFrameRateRangeMake(60,60,60);++fpsEpoch;
    }else if(row==1){self.fpsEnabled=!self.fpsEnabled;[NSUserDefaults.standardUserDefaults setBool:self.fpsEnabled forKey:@"recomp.fps"];++fpsEpoch;}
    else{[self toggleRecomp];return;}
    [self showRecompPage];
}
- (void)menuInput:(uint32_t)pressed {
    if(self.menuPage==2){for(unsigned i=0;i<13;++i)if(pressed&(1u<<i)){ios_controller_assign(i,ios_bindings::targets[self.bindingTarget]);self.menuPage=1;self.menuIndex=self.bindingTarget;[self showRecompPage];return;}return;}
    NSInteger n=self.menuButtons.count;if(!n)return;
    if(pressed&(1u<<7))self.menuIndex=(self.menuIndex+n-1)%n;
    if(pressed&(1u<<8))self.menuIndex=(self.menuIndex+1)%n;
    if(pressed&1u)[self activateMenuRow:self.menuIndex];
    else if(pressed&2u){if(self.menuPage){self.menuPage=0;self.menuIndex=2;[self showRecompPage];}else [self toggleRecomp];}
    [self highlightMenu];
}

