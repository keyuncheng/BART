if (T) {
    x1<-read.table("../../../data/simulation/230708/exp_a3_3.dat", header=TRUE)
    library(ggplot2)
    library(grid)
    library(extrafont)
    font_import()
    loadfonts()

    ggplot(x1, aes(x=num_nodes_id, y=max_load, group=method_id, shape=method_id, color=method_id)) +
    scale_color_manual(name=element_blank(),
                        breaks=c("a","b","c"),
                        labels=c("RD","BW","BART"),
                        values=c(a='#1E90FF',b='#C1A61D',c='#EE1111')) +
    scale_shape_manual(name=element_blank(),
                        breaks=c("a","b","c"),
                        labels=c("RD","BW","BART"),
                        values=c(1,4,5)) +
    scale_linetype_manual(name=element_blank(),
                        breaks=c("a","b","c"),
                        labels=c("RD","BW","BART"),
                        values=c(2,3,4)) +
    scale_x_discrete(breaks=c("a","b","c","d"),
                    labels=c("100", "200", "300", "400")) +
    scale_y_continuous(expand = c(0,0),limits=c(0,520), breaks=c(0,100,200,300,400,500)) +
    geom_errorbar(aes(ymin=max_load_min, ymax=max_load_max, color=method_id), width=.1) +

    geom_line(linewidth=1, aes(linetype=method_id)) +
    geom_point(size=5, stroke=1, fill="white") +
    guides(color=guide_legend(ncol=1, byrow=TRUE, keywidth=0.8, keyheight=0.8), 
           linetype = guide_legend(override.aes = list(size = 3))) + # Increase the size of the line in the legend
    ylab("Max Load (in Blocks)") +
    xlab(expression(italic(paste("N")))) +
    theme_classic() +
    theme(axis.text.x = element_text(size=20, angle=0, hjust=0.5, vjust=1, color="black", family="Times New Roman")) +
    theme(axis.text.y = element_text(size=20, color="black", family="Times New Roman")) +
    theme(axis.title.x = element_text(size=20, family="Times New Roman")) +
    theme(axis.title.y = element_text(size=20, family="Times New Roman")) +
    theme(legend.text = element_text(size=20, family="Times New Roman")) +
    theme(legend.position=c(0.8,0.8), legend.direction = "horizontal", legend.key=element_blank())
    ggsave("../../../pdf/simulation/230708/exp_a3_3.pdf", width=5.5, height=4, device=cairo_pdf)
}