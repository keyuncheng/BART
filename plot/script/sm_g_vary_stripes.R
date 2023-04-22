# sm_g tasks
 if (T) {
     x1<-read.table("../data/sm_g_vary_stripes.dat", header=TRUE)
     library(ggplot2)
     library(grid)
     library(extrafont)
     font_import()
     loadfonts()
 
     ggplot(x1, aes(x=num_stripes_id, y=avg_num_send_tasks, fill=ec_param_id)) +
     scale_fill_manual(name=element_blank(),
                       breaks=c("a","b"),
                       labels=c("(9,6)","(14,10)"),
                       #values=c(b='#3366ff',c='#33cccc',d='#999966')) +
                       values=c(a="turquoise2",b="tan")) +
     scale_x_discrete(breaks=c("a","b","c","d"),
                      labels=c("1000", "2000", "5000", "10000")) +
     scale_y_continuous(expand = c(0,0),limits=c(0,310), breaks=c(0,50,100,150,200,250,300)) +
     guides(fill=guide_legend(nrow=3, byrow=TRUE, keywidth=1,keyheight=1)) +
     geom_bar(color="black", stat="identity", width=0.8, position=position_dodge()) +
 	geom_errorbar(aes(ymin=min, ymax=max),width=.2, position=position_dodge(.8)) +
     ylab("# of Send Tasks") +
     xlab("# of Stripes") +
     theme_classic() +
     theme(axis.text.x = element_text(size=12, angle=0, hjust=0.5, vjust=1, color="black")) +
     theme(axis.text.y = element_text(size=15, color="black")) +
     theme(legend.text = element_text(size=15)) +
     theme(axis.title.x = element_text(size=15)) +
     theme(axis.title.y = element_text(size=15)) +
     theme(legend.position=c(0.3,0.85), legend.direction="vertical", legend.key=element_blank())
     ggsave("../pdf/sm_g_vary_stripes.pdf", width=4, height=2.7,device=cairo_pdf)
 }
