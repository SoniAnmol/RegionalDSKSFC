library(foreach)
library(doParallel)

#Specify number of cores
ncores<-detectCores()
#Set up the parallel cluster
cl<-makeCluster((ncores), outfile="")
registerDoParallel(cl)

#Modify below to specify the path to the input file
inputfile="[PATH]/dsk_sfc_inputs.json"
#Modify below to load in the function to change the input file
source("[PATH]/fnModifyInputs.R")

#Number of seeds to run
MC=108
#Name for the run
runname=paste("-r partest")
#Modify below to specify the names of the parameters/initial values/flags etc to change and insert the desired values
modifyInputs(c("name1","name2","..."),c(value1,value2,...,),inputfile)
#Note that changes you make here to dsk_sfc_inputs.json are permanent, i.e. the values will not by themselves change back to the previous ones after the run!
#You need to do this either manually or by once more invoking modifyInputs once the run has finished

#Enter the parallel loop
foreach(j=1:MC, .errorhandling = 'remove', .multicombine=TRUE) %dopar% {
  
  #Here the seed is given by j, i.e. we will simulate seeds 1 to MC
  seed=paste("-s ", j)
  #Get the path to the executable
  exec=file.path(".","dsk_SFC")
  #Paste together the console command to call the executable with the right input file, run name and seed
  command=paste(exec,inputfile,runname,seed)
  #Call the executable
  system(command)
  
}

#Stop the cluster
stopCluster(cl)