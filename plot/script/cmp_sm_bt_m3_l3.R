# sm_g tasks
 if (T) {
     x1<-read.table("../data/preliminary/cmp_sm_bt_m3_l3.dat", header=TRUE)
     library(ggplot2)
     library(grid)
     library(extrafont)
     font_import()
     loadfonts()
 
     ggplot(x1, aes(x=code_param_id, y=max_load, fill=method_id)) +
     scale_fill_manual(name=element_blank(),
                       breaks=c("a","b"),
                       labels=c("SM","BT"),
                       #values=c(b='#3366ff',c='#33cccc',d='#999966')) +
                       values=c(a="turquoise2",b="tan")) +
     scale_x_discrete(breaks=c("a","b","c","d","e","f"),
                      labels=c("3", "4", "5", "6", "8", "16")) +
     scale_y_continuous(expand = c(0,0),limits=c(0,41), breaks=c(0,10,20,30,40)) +
     guides(fill=guide_legend(nrow=1, byrow=TRUE, keywidth=1,keyheight=1)) +
     geom_bar(color="black", stat="identity", width=0.8, position=position_dodge()) +
 	# geom_errorbar(aes(ymin=min, ymax=max),width=.2, position=position_dodge(.8)) +
     ylab("Max load") +
     xlab("k") +
     theme_classic() +
     theme(axis.text.x = element_text(size=12, angle=0, hjust=0.5, vjust=1, color="black")) +
     theme(axis.text.y = element_text(size=15, color="black")) +
     theme(legend.text = element_text(size=15)) +
     theme(axis.title.x = element_text(size=15)) +
     theme(axis.title.y = element_text(size=15)) +
     theme(legend.position=c(0.8,0.96), legend.direction="vertical", legend.key=element_blank())
     ggsave("../pdf/cmp_sm_bt_m3_l3.pdf", width=4, height=2.7,device=cairo_pdf)
 }
