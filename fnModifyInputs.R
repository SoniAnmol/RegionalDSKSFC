modifyInputs<-function(inputNames,inputValues,filepath){

  library(jsonlite)
  #Read the json file
  inputs=fromJSON(filepath)
  
  parnames=names(inputs$params)
  climparnames=names(inputs$climparams)
  climshockparnames=names(inputs$climshockparams)
  flagnames=names(inputs$flags)
  initnames=names(inputs$inits)
  
  for(i in 1:length(inputNames)){
    if(is.element(inputNames[i],parnames)){
      index=which(names(inputs$params)==inputNames[i])
      inputs$params[index]=inputValues[i]
    }
    if(is.element(inputNames[i],climparnames)){
      index=which(names(inputs$climparams)==inputNames[i])
      inputs$climparams[index]=inputValues[i]
    }
	if(is.element(inputNames[i],climshockparnames)){
      index=which(names(inputs$climshockparams)==inputNames[i])
      inputs$climshockparams[index]=inputValues[i]
    }
    if(is.element(inputNames[i],flagnames)){
      index=which(names(inputs$flags)==inputNames[i])
      inputs$flags[index]=inputValues[i]
    }
    if(is.element(inputNames[i],initnames)){
      index=which(names(inputs$inits)==inputNames[i])
      inputs$inits[index]=inputValues[i]
    }
  }
  
  inputs<-toJSON(inputs, pretty = TRUE, auto_unbox = TRUE, digits = 8)
  write(inputs,filepath)
}